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

#pragma once

#include <QIODevice>
#include <QBuffer>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QFile>
#include "thirdparty/fftcalc.h"

class AudioStreamDevice : public QIODevice
{
    Q_OBJECT

public:
    explicit AudioStreamDevice(QObject *parent = nullptr);
    bool init(const QAudioFormat& format);
    bool uninit();

    enum State { Playing, Stopped };

    void play(const QUrl &file);
    void stop();

    bool atEnd() const override;

    QVector<double> spectrum() const;

    void calculateSpectrum(QAudioBuffer buffer);

    qint64 duration() const;
    qint64 position() const;

    void seekFrom(qint64 position);
    void handleError(QAudioDecoder::Error error);

protected:
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char* data, qint64 len) override;

private:
    QFile m_file;
    QBuffer m_input;
    QBuffer m_output;
    QByteArray m_data;
    QAudioDecoder m_decoder;
    QAudioFormat m_format;
    FFTCalc *calculator;
    QVector<double> sample;
    QVector<double> m_spectrum;
    double levelLeft, levelRight;
    qint64 m_calculatedDuration;

    State m_state;

    bool isInited;
    bool isDecodingFinished;

    void clear();

private slots:
    void bufferReady();
    void finished();

signals:
    void stateChanged(AudioStreamDevice::State state);
    void newData(const QByteArray& data);
    int levels(double left, double right);
    void spectrumChanged(QVector<double> spectrum);
    void invalidMedia();
    void endOfMedia();
    void bufferingMedia();
    void bufferedMedia();
    void stalledMedia();
    void decodeFinished();
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void errorDecoding(QAudioDecoder::Error error);
};
