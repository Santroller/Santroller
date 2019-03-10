pipeline {
    agent { label 'Main' }
    properties([parameters(generateParams())]) 
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
    env.getEnvironment().each { name, value -> file.replace("#define ${name}.*\n", "#define ${name} ${value}\n")}
    return file
}

def generateParams() {
    def list = []

    if (env.BRANCH_NAME != 'master') {
        readFile("src/config/config.h").eachLine { line ->
            if (line.startsWith("#define")) {
                def split = line.split(" ")
                list << stringParam(description: '', name: split[1], defaultValue: split[2])
            }
        }
        
    }
    return list
}