/*
 * Copyright 2023 by Aditya Mehra <aix.m@outlook.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "audiostreamdevice.h"
#include <QUrl>
#include <QFile>

AudioStreamDevice::AudioStreamDevice(QObject *parent):
    m_input(&m_data),
    m_output(&m_data),
    m_state(State::Stopped)
{
    setOpenMode(QIODevice::ReadOnly);

    isInited = false;
    isDecodingFinished = false;
    calculator = new FFTCalc(this);
    connect(calculator, &FFTCalc::calculatedSpectrum, this, [this](QVector<double> spectrum) {
        int size = 20;
        m_spectrum.resize(size);
        int j = 0;
        for (int i = 0; i < spectrum.size(); i += spectrum.size()/(size - 1)) {
           m_spectrum[j] = spectrum[i];
           ++j;
        }
        emit spectrumChanged(m_spectrum);
    });
    connect(&m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this, &AudioStreamDevice::handleError);
}

bool AudioStreamDevice::uninit()
{
    if (isInited)
    {
        m_decoder.stop();
        m_input.reset();
        m_output.reset();
        m_data.clear();
        m_spectrum.clear();
        isInited = false;
        isDecodingFinished = false;
        return true;
    }
    return false;
}

bool AudioStreamDevice::init(const QAudioFormat& format)
{
    m_format = format;
    m_decoder.setAudioFormat(m_format);

    connect(&m_decoder, SIGNAL(bufferReady()), this, SLOT(bufferReady()));
    connect(&m_decoder, SIGNAL(finished()), this, SLOT(finished()));

    if (!m_output.open(QIODevice::ReadOnly) || !m_input.open(QIODevice::WriteOnly))
    {
        return false;
    }

    isInited = true;

    return true;
}

qint64 AudioStreamDevice::readData(char* data, qint64 maxlen)
{
    memset(data, 0, maxlen);

    if (m_state == State::Playing)
    {
        m_output.read(data, maxlen);

        if (maxlen > 0)
        {
            QByteArray buff(data, maxlen);
            emit newData(buff);
            QAudioBuffer buffer(buff, m_format);
            calculateSpectrum(buffer);
            emit positionChanged(m_output.pos());
        }

        if (atEnd())
        {
            stop();
            emit endOfMedia();
        }
    }

    return maxlen;
}

qint64 AudioStreamDevice::writeData(const char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

void AudioStreamDevice::play(const QUrl &file)
{
     clear();
     m_decoder.setSource(file);
     m_decoder.start();
     qDebug() << "m_decoder started";
     m_state = State::Playing;
     emit stateChanged(m_state);
}

void AudioStreamDevice::stop()
{
    clear();
    m_state = State::Stopped;
    emit stateChanged(m_state);
}


void AudioStreamDevice::clear()
{
    m_decoder.stop();
    m_data.clear();
    isDecodingFinished = false;
    qDebug() << "In Clear Decoder";
}

bool AudioStreamDevice::atEnd() const
{
    return m_output.size()
        && m_output.atEnd()
        && isDecodingFinished;
}

void AudioStreamDevice::bufferReady()
{
    const QAudioBuffer &buffer = m_decoder.read();

    const int length = buffer.byteCount();
    const char *data = buffer.constData<char>();

    m_input.write(data, length);
}

void AudioStreamDevice::finished()
{
    isDecodingFinished = true;
    emit durationChanged(m_output.size());
    if(m_output.size() == m_input.size()) {
        emit bufferedMedia();
    }
}

QVector<double> AudioStreamDevice::spectrum() const
{
    return m_spectrum;
}


void AudioStreamDevice::calculateSpectrum(QAudioBuffer buffer)
{
    qreal peakValue;
    int duration;

    if(buffer.frameCount() < 512)
        return;

    levelLeft = levelRight = 0;

    if(buffer.format().channelCount() != 2)
        return;

    sample.resize(buffer.frameCount());
    if(buffer.format().sampleFormat() == QAudioFormat::UInt8){
        QAudioBuffer::S16S *data = (QAudioBuffer::S16S *)buffer.constData<char>();
        peakValue=UINT8_MAX/2;
        for(int i=0; i<buffer.frameCount(); i++){
            sample[i] = data[i].value(QAudioFormat::FrontLeft) / peakValue;
            levelLeft += data[i].value(QAudioFormat::FrontLeft);
            levelRight += data[i].value(QAudioFormat::FrontRight);
        }
    } else if(buffer.format().sampleFormat() == QAudioFormat::Int16){
        QAudioBuffer::S16S *data = (QAudioBuffer::S16S *)buffer.constData<char>();
        peakValue=INT16_MAX;
        for(int i=0; i<buffer.frameCount(); i++){
            sample[i] = data[i].value(QAudioFormat::FrontLeft) / peakValue;
            levelLeft += data[i].value(QAudioFormat::FrontLeft);
            levelRight += data[i].value(QAudioFormat::FrontRight);
        }
    } else if(buffer.format().sampleFormat() == QAudioFormat::Int32){
        QAudioBuffer::S32S *data = (QAudioBuffer::S32S *)buffer.constData<char>();
        peakValue=INT32_MAX;
        for(int i=0; i<buffer.frameCount(); i++){
            sample[i] = data[i].value(QAudioFormat::FrontLeft) / peakValue;
            levelLeft += data[i].value(QAudioFormat::FrontLeft);
            levelRight += data[i].value(QAudioFormat::FrontRight);
        }
    } else if(buffer.format().sampleFormat() == QAudioFormat::Float){
        QAudioBuffer::F32S *data = (QAudioBuffer::F32S *)buffer.constData<char>();
        peakValue=1.0;
        for(int i=0; i<buffer.frameCount(); i++){
            sample[i] = data[i].value(QAudioFormat::FrontLeft) / peakValue;
            levelLeft += data[i].value(QAudioFormat::FrontLeft);
            levelRight += data[i].value(QAudioFormat::FrontRight);
        }
    } else {
        return;
    }

    duration = buffer.format().durationForBytes(buffer.frameCount())/1000;
    calculator->calc(sample, duration);
    emit levels(levelLeft/buffer.frameCount(), levelRight/buffer.frameCount());
}

qint64 AudioStreamDevice::duration() const
{
    return m_output.size();
}

qint64 AudioStreamDevice::position() const
{   
    return m_output.pos();
}

void AudioStreamDevice::seekFrom(qint64 position)
{
    m_output.seek(position);
}

void AudioStreamDevice::handleError(QAudioDecoder::Error error)
{
    qDebug() << "Error: " << error;
}
