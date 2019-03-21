pipeline {
    agent { label 'Main' }
    parameters {
        string(name: 'POLL_RATE', defaultValue: '1', description: '')
        string(name: 'MPU_6050', defaultValue: '1', description: '')
        string(name: 'WII', defaultValue: '1', description: '')
        string(name: 'DIRECT', defaultValue: '2', description: '')
        string(name: 'XINPUT', defaultValue: '1', description: '')
        string(name: 'KEYBOARD', defaultValue: '2', description: '')
        string(name: 'TILT_SENSOR', defaultValue: 'MPU_6050', description: '')
        string(name: 'DEVICE_TYPE', defaultValue: 'DIRECT', description: '')
        string(name: 'OUTPUT_TYPE', defaultValue: 'XINPUT', description: '')
        string(name: 'DPAD', defaultValue: '1', description: '')
        string(name: 'JOY', defaultValue: '2', description: '')
        string(name: 'PIN_GREEN_FRET', defaultValue: '4', description: '')
        string(name: 'PIN_RED_FRET', defaultValue: '5', description: '')
        string(name: 'PIN_YELLOW_FRET', defaultValue: '7', description: '')
        string(name: 'PIN_BLUE_FRET', defaultValue: '6', description: '')
        string(name: 'PIN_ORANGE_FRET', defaultValue: '8', description: '')
        string(name: 'PIN_START_BUTTON', defaultValue: '16', description: '')
        string(name: 'PIN_SELECT_BUTTON', defaultValue: '9', description: '')
        string(name: 'PIN_WHAMMY_POTENIOMETER', defaultValue: '18', description: '')
        string(name: 'PIN_STRUM_UP_BUTTON', defaultValue: '14', description: '')
        string(name: 'PIN_STRUM_DOWN_BUTTON', defaultValue: '15', description: '')
        string(name: 'PIN_DPAD_LEFT_BUTTON', defaultValue: '10', description: '')
        string(name: 'PIN_DPAD_RIGHT_BUTTON', defaultValue: '21', description: '')
        string(name: 'PIN_JOYSTICK_X_POTENIOMETER', defaultValue: '19', description: '')
        string(name: 'PIN_JOYSTICK_Y_POTENIOMETER', defaultValue: '20', description: '')
        string(name: 'DIRECTION_MODE', defaultValue: 'JOY', description: '')
        string(name: 'INVERT_WHAMMY', defaultValue: 'true', description: '')
        string(name: 'WHAMMY_INITIAL_VALUE', defaultValue: '16863', description: '')
        string(name: 'FRETS_LED', defaultValue: '1', description: '')
        string(name: 'KEY_GREEN_FRET', defaultValue: 'HID_KEYBOARD_SC_A', description: '')
        string(name: 'KEY_RED_FRET', defaultValue: 'HID_KEYBOARD_SC_S', description: '')
        string(name: 'KEY_YELLOW_FRET', defaultValue: 'HID_KEYBOARD_SC_J', description: '')
        string(name: 'KEY_BLUE_FRET', defaultValue: 'HID_KEYBOARD_SC_K', description: '')
        string(name: 'KEY_ORANGE_FRET', defaultValue: 'HID_KEYBOARD_SC_L', description: '')
        string(name: 'KEY_WHAMMY', defaultValue: 'HID_KEYBOARD_SC_SEMICOLON_AND_COLON', description: '')
        string(name: 'KEY_START', defaultValue: 'HID_KEYBOARD_SC_ENTER', description: '')
        string(name: 'KEY_SELECT', defaultValue: 'HID_KEYBOARD_SC_H', description: '')
        string(name: 'KEY_LEFT', defaultValue: 'HID_KEYBOARD_SC_LEFT_ARROW', description: '')
        string(name: 'KEY_RIGHT', defaultValue: 'HID_KEYBOARD_SC_RIGHT_ARROW', description: '')
        string(name: 'KEY_UP', defaultValue: 'HID_KEYBOARD_SC_UP_ARROW', description: '')
        string(name: 'KEY_DOWN', defaultValue: 'HID_KEYBOARD_SC_DOWN_ARROW', description: '')
        string(name: 'PIN_GRAVITY', defaultValue: '11', description: '')
        string(name: 'FLIP_MPU_6050', defaultValue: '0', description: '')
        string(name: 'MPU_6050_START', defaultValue: '28000', description: '')
        string(name: 'F_CPU', defaultValue: '16000000', description: '')
    }
    stages {
        stage('Build') {
            steps {
                writeFile file: "src/config/config.h", text: updateParams(readFile("src/config/config.h"))
                writeFile file: "src/micro/makefile", text: readFile("src/micro/makefile").replaceAll(/\nF_CPU.*\n/,"\nF_CPU=${env.F_CPU}\n")
                sh 'make build' 
                archiveArtifacts artifacts: "src/micro/bin/Ardwiino.hex", fingerprint: true 
            }
        }
    }
}

@NonCPS
def updateParams(file) {
    env.getEnvironment().each { name, value -> file = file.replaceAll(/#define ${name} .*\n/, "#define ${name} ${value}\n")}
    return file
}