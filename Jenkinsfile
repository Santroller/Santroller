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
        string(name: 'DEVICE_TYPE', defaultValue: 'WII', description: '')
        string(name: 'OUTPUT_TYPE', defaultValue: 'XINPUT', description: '')
        string(name: 'DPAD', defaultValue: '1', description: '')
        string(name: 'JOY', defaultValue: '2', description: '')
        string(name: 'PIN_GREEN', defaultValue: '4', description: '')
        string(name: 'PIN_RED', defaultValue: '5', description: '')
        string(name: 'PIN_YELLOW', defaultValue: '7', description: '')
        string(name: 'PIN_BLUE', defaultValue: '6', description: '')
        string(name: 'PIN_ORANGE', defaultValue: '8', description: '')
        string(name: 'PIN_START', defaultValue: '16', description: '')
        string(name: 'PIN_SELECT', defaultValue: '9', description: '')
        string(name: 'PIN_LEFT', defaultValue: '10', description: '')
        string(name: 'PIN_RIGHT', defaultValue: '21', description: '')
        string(name: 'PIN_UP', defaultValue: '14', description: '')
        string(name: 'PIN_DOWN', defaultValue: '15', description: '')
        string(name: 'PIN_WHAMMY', defaultValue: '18', description: '')
        string(name: 'PIN_JOY_X', defaultValue: '19', description: '')
        string(name: 'PIN_JOY_Y', defaultValue: '20', description: '')
        string(name: 'MOVE_MODE', defaultValue: 'JOY', description: '')
        string(name: 'WHAMMY_DIR', defaultValue: '-1', description: '')
        string(name: 'WHAMMY_START', defaultValue: '16863', description: '')
        string(name: 'FRETS_LED', defaultValue: '1', description: '')
        string(name: 'KEY_GREEN', defaultValue: 'HID_KEYBOARD_SC_A', description: '')
        string(name: 'KEY_RED', defaultValue: 'HID_KEYBOARD_SC_S', description: '')
        string(name: 'KEY_YELLOW', defaultValue: 'HID_KEYBOARD_SC_J', description: '')
        string(name: 'KEY_BLUE', defaultValue: 'HID_KEYBOARD_SC_K', description: '')
        string(name: 'KEY_ORANGE', defaultValue: 'HID_KEYBOARD_SC_L', description: '')
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

    }
    stages {
        stage('Build') {
            steps {
                writeFile file: "src/config/config.h", text: updateParams(readFile("src/config/config.h"))
                writeFile file: "src/micro/makefile", text: readFile("src/micro/makefile").replace("F_CPU.*\n","F_CPU=${env.F_CPU}")
                sh 'make build' 
                archiveArtifacts artifacts: "src/micro/bin/Ardwiino.hex, src/config/config.h", fingerprint: true 
            }
        }
    }
}

@NonCPS
def updateParams(file) {
    print file
    print env.getEnvironment()
    env.getEnvironment().each { name, value -> file = file.replace("#define ${name}.*\n", "#define ${name} ${value}\n")}
    print file
    return file
}