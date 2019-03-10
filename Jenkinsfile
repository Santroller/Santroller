pipeline {
    agent any

    stages {
        stage('Build') {
            steps {
                writeFile file: 'src/config/config.h', text: updateParams(readFile 'src/config/config.h')
                writeFile file: 'src/micro/makefile', text: (readFile 'src/micro/makefile').replace("F_CPU.*\n","F_CPU=${env.F_CPU}")
                sh 'make build' 
                archiveArtifacts artifacts: 'src/micro/bin/Ardwiino.hex', fingerprint: true 
            }
        }
    }
}

@NonCPS
def updateParams(file) {
    env.getEnvironment().each { name, value -> file.replace("#define ${name}.*\n", "#define ${name} ${value}\n");
    return file
}