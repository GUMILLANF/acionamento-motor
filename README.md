# acionamento-motor

**1. Introdução**

O objetivo deste projeto de eletrônica é desenvolver um sistema de acionamento remoto de um motor utilizando o protocolo MQTT (Message Queuing Telemetry Transport), com a utilização de um cartão RFID devidamente validado. Somente um cartão RFID registrado será capaz de acionar e parar o motor. Além disso, o projeto prevê o desligamento do motor em caso de aumento elevado de temperatura. Para a implementação, foram montados três módulos, cada um com suas funções específicas, e foi utilizado um banco de dados PostgreSQL para registrar os IDs dos cartões RFID lidos e seus respectivos status.

**2. Materiais Utilizados**

Os seguintes materiais foram utilizados na montagem do projeto:

Módulo 1: ESP32, leitor RFID RC522 e um botão Push Button para alternar o modo do módulo entre leitura e gravação.
Módulo 2: ESP32, relé para acionamento do motor através do Módulo 3, outro relé para inverter o giro do motor, LEDs indicadores de estado da conexão Wi-Fi e servidor MQTT, display LCD para mostrar o estado da conexão Wi-Fi, estado do servidor MQTT e temperatura ambiente (obtida através de um sensor LM35).
Módulo 3: Ponte H que será acionada via relé do Módulo 2. Possui um botão Push Button que possibilita a inversão do giro do motor.
 
**3. Funcionamento do Sistema((

O sistema é composto por três módulos distintos que trabalham em conjunto para o acionamento do motor e o monitoramento de temperatura. O fluxo de funcionamento do sistema é descrito a seguir:

**3.1. Módulo 1**

O Módulo 1 contém o ESP32, o leitor RFID RC522 e um botão Push Button. Ele é responsável pela leitura dos cartões RFID e pela validação dos mesmos. O botão Push Button permite alternar o módulo entre o modo de leitura e gravação. Quando um cartão RFID válido é lido, o Módulo 1 atualiza o tópico no servidor MQTT e também registra o ID do cartão no banco de dados, juntamente com o status de ativação.

**3.2. Módulo 2**

O Módulo 2 possui um ESP32, relé de acionamento do motor através do Módulo 3, relé para inverter o giro do motor, LEDs indicadores de estado da conexão Wi-Fi e servidor MQTT, e um display LCD. Esse módulo é configurado para publicar e ler no mesmo tópico no servidor MQTT, protegido por usuário e senha. Quando há uma atualização nesse tópico, o Módulo 2 atualiza seu estado, acionando o relé correspondente para ligar o motor por meio da Ponte H.

O Módulo 2 também possui um sensor de temperatura LM35, que mede a temperatura ambiente. Essa informação é exibida no display LCD, juntamente com o estado da conexão Wi-Fi e do servidor MQTT. Caso ocorra um aumento elevado de temperatura, o motor será desligado, e a ligação do motor só será possível novamente após a temperatura diminuir para níveis seguros.

**3.3. Módulo 3**

O Módulo 3 é composto pela Ponte H, que é acionada pelo relé do Módulo 2. Além disso, possui um botão Push Button que possibilita a inversão do giro do motor.

**3.4. Dashboard de Controle**

O dashboard de controle criado no Node-Red( Ferramenta de desenvolvimento baseada em fluxo de dados ), possuí dois botões, um para ligar o motor e outro para desligar o motor, ambos ao ser clicado atualiza o tópico no servidor MQTT, e assim liga ou desliga o motor remotamente.

Pelo dashboar também é possível visualizar o status em que o motor se encontra, possuí também um alerta para quando a temperatura está alta, e por último é exibido o histório dos acionamentos e desligamentos do motor, mostrando o horário e se o motor foi desligado devido a temperatura ou não.

**4. Banco de Dados, API e Node-Red**

Foi utilizado o banco de dados PostgreSQL para armazenar os IDs dos cartões RFID lidos e seus respectivos status de ativação. Essas informações são registradas sempre que um cartão válido é lido pelo Módulo 1. Para facilitar as operações de atualização do status do cartão RFID, foi criado chamadas HTTP, com todo o fluxo das regras controlada pelo Node-Red.

**5.Considerações Finais**

O projeto de acionamento remoto de motor com RFID, MQTT, banco de dados e API apresentado neste relatório demonstra a utilização de diferentes módulos eletrônicos interconectados para permitir o controle seguro e remoto de um motor. O sistema utiliza o protocolo MQTT para a comunicação entre os módulos, garantindo a transmissão segura das informações. Além disso, o uso de um banco de dados PostgreSQL permite o registro e a atualização dos cartões RFID válidos, facilitando o gerenciamento do sistema.
