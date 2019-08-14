pipeline {
    agent { dockerfile true }
    stages {
        stage('Pull Docker') {
            steps {
                script {
                    docker.image("ghostofcookie/gofish:latest").pull()
                }
            }
        }
        stage('Build Docker') {
            steps {
                script {
                    docker.build('ghostofcookie/gofish:latest')
                }
            }
        }
        stage('Build') {
            steps {
                script {
                    docker.image("ghostofcookie/gofish:latest").inside() {
                        sh "/goFish/build.sh"
                    }
                }
            }
        }
        stage('Test') {
            steps {
                script {
                    docker.image("ghostofcookie/gofish:latest").inside() {
                        sh "/goFish/run_tests.sh"
                    }
                }
            }
        }
    }
}