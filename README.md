Mycroft GUI
===========

Graphical User Interface powered by Qt and Kirigami for [Mycroft AI](https://github.com/MycroftAI/mycroft-core)

This fork represents the last commit of mycroft-gui licensed under Apache2.0, the upstream repository is now licensed as GPL3.0 and no further updates will be shared with the OVOS ecosystem

Eventually this fork will evolve into a ovos branded equivalent code base, we had not intention of forking and if upstream reverts their licensing changes this repo will be archived

The main intention of this fork is to be a dependency of ovos-shell
_______________________________________________________________________

- [Getting Started](#getting-started)
  + [General setup instructions](#general-setup-instructions)
  + [Virtual environment setup instructions](#virtual-environment-setup-instructions)
- [Usage](#usage)
  + [Local environment](#local-environment)
- [Development & Contributing](#development-contributing)
  + [Contributing to Mycroft GUI API](#contributing-to-mycroft-gui-api)
  + [Developing skills with an User Interface](#developing-skills-with-an-user-interface)
- [Troubleshooting](#troubleshooting)
  - [Debugging Mycroft GUI](#debugging-mycroft-gui)

_____________________________________________________________________________________________

## Getting Started

The following guide will provide a set of instructions to get you started with installing Mycroft GUI on your system. *Note:* All non virtual environment setups should generally follow the [General Setup Instructions](#general-setup-instructions) steps. 

### 

### General Setup Instructions

The interactive Installation script supports installation on KDE Neon, K/Ubuntu 20.04 & Manjaro Linux, It also supports installation and building on other distributions and platforms but system dependencies will be have to installed manually as mentioned in the Installation Script.



1) Fetch Repository and Run Installation Script 
   
   ```@bash
    cd ~
    git clone https://github.com/mycroftai/mycroft-gui
    cd mycroft-gui
    bash dev_setup.sh    
   ```

2) Running Mycroft GUI:
   
   ```@bash
   mycroft-gui-app
   ```

### Virtual Environment Setup Instructions

1) Download and Install [VirtualBox](https://www.virtualbox.org/wiki/Downloads)

2) Download Latest (Any Supported) Distribution ISO.

3) Open VirtualBox and create a system installation

4) Within the VM, open a terminal session and type:
   
   ```@bash
    cd ~
    git clone https://github.com/mycroftai/mycroft-gui
    cd mycroft-gui
    bash dev_setup.sh
   ```

5) (Optional) Customize Plasma settings for GUI debugging purposes
   
   * Disable screensaver
     - Click the > icon, type: Screen Locking
     - Untick "Lock screen automatically after:"
     - Click OK
6. Running Mycroft GUI:
   
   ```@bash
   mycroft-gui-app
   ```

## 

## Usage

Note: ovos-core must be running first

1) Invoke using ```mycroft-gui-app``` in any terminal, or Mycroft icon from your desktop application launcher.

2) Click *Start* button in the middle of the window if Autoconnect is disabled

3) Talk to your Mycroft!

## Development & Contributing

Users & Developers can contribute in advancement of Mycroft GUI in several ways, below mentioned methods will help you identify contribution targets.

### Contributing To Mycroft GUI API

Mycroft GUI API is the protocol layer designed to talk and integrate with Mycroft Core Services, It handles various functions some of which include setting, managing and parsing session data and handling delegate activities for skill instances, API Implementation documentation can be found at:

- [Transport Protocol Documentation](https://github.com/MycroftAI/mycroft-gui/blob/master/transportProtocol.md)

- [Enclosure API Implementation - Mycroft Core](https://github.com/MycroftAI/mycroft-core/blob/dev/mycroft/enclosure/gui.py)

### Developing Skills With An User Interface

Mycroft enabled devices with displays such as the Mark II, KDE Plasmoid provide skill developers the opportunity to create skills that can be empowered by both voice and screen interaction. The display interaction technology is based on the QML user interface markup language that gives you complete freedom to create in-depth innovative interactions without boundaries, A detailed documentation on getting started with developing skills with an user Interface is available at [Mycroft Documentation - Displaying Information](https://mycroft-ai.gitbook.io/docs/skill-development/displaying-information/mycroft-gui)

## Troubleshooting

Having trouble or facing issues with Mycroft GUI ? You can use the below guide to debug Mycroft GUI

### Debugging Mycroft GUI

1. **Debugging Mycroft GUI Application:**
   
   - Run Mycroft GUI application from a terminal
   
   - Errors should be visible on teriminal output, copy output to a log file
   
   - Open a relevant issue and post your log file 

2. **Debugging Skills & Session Data**:
   
   - Start Mycroft CLI client and use ctrl+g to view session data being passed to Mycroft GUI
   
   - For more detailed logging add { "log_level": "debug"} to your mycroft system configuration
   
   - More detailed logs can be retrived from "/var/logs/mycroft/enclosure.log" & "/var/logs/mycroft/skills.log"

3. **Debugging on Remote Device (Example: Mark-1)**:
   
   - SSH to the Mark 1
   
   - Run `sudo service mycroft-enclosure-client stop`
   
   - Run `sudo su mycroft`
   
   - Run `mycroft-enclosure-client` This will start the client and show Debug() messages on the console.

## 
