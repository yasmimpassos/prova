#include <WiFi.h>
#include <HTTPClient.h>

#define LED_VERDE 2 // Pino utilizado para controle do led verde
#define LED_VERMELHO 40 // Pino utilizado para controle do led vermelho
#define LED_AMARELO 9 // Pino utilizado para controle do led amarelo
#define BOTAO 18 // Pino utilizado pelo botao
#define LDR 4 // Pino utilizado pelo ldr

int estado_botao = 0;  // variavel para armazenar o estado do botao
bool botao_apertado = false;

int limite_ldr = 600; // define o limite da luminosidade para considerar como dia ou noite

// variaveis para controlar o tempo
long tempo_decorrido, tempo_anterior, ultimo_tempo_debounce;

// variavel para controlar quanto tempo o led vai ficar ligado
int intervalo = 0; // inicializa com o de 0 segundos, para o led verde ligar imediatamente
int intervalo_debounce = 50; // intervalo para resolver o problema do bouncing

// variaveis para controlar se está no modo noturno
bool modo_noturno, modo_noturno_ativo = false;


// variavel para armazenar quantas vezes o botao foi apertado
int contagem_botao = 0;

// Estados do semáforo
enum EstadosSemaforo {
  ESTADO_VERMELHO,
  ESTADO_AMARELO,
  ESTADO_VERDE,
  ESTADO_MODO_NOTURNO
};

EstadosSemaforo proximo_estado = ESTADO_VERDE; // Estado inicial
HTTPClient http;

void setup() {

  // Configuração inicial dos pinos para controle dos leds como OUTPUTs (saídas) do ESP32
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);

  // Inicialização das entradas
  pinMode(BOTAO, INPUT); // Inicializa o botao como input
  pinMode(LDR, INPUT); // Inicializa o ldr como input

  // Inicializa com todos os leds desligados
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);

  Serial.begin(9600); // Configuração para debug por interface serial entre ESP e computador com baud rate de 9600

  WiFi.begin("Wokwi-GUEST", ""); // Conexão à rede WiFi aberta com SSID Wokwi-GUEST

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("Conectado ao WiFi com sucesso!"); // Considerando que saiu do loop acima, o ESP32 agora está conectado ao WiFi (outra opção é colocar este comando dentro do if abaixo)

  if (WiFi.status() == WL_CONNECTED) { // Se o ESP32 estiver conectado à Internet

    String serverPath = "http://www.google.com.br/"; // Endpoint da requisição HTTP

    http.begin(serverPath.c_str());

    int httpResponseCode = http.GET(); // Código do Resultado da Requisição HTTP

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }

  else {
    Serial.println("WiFi Disconnected");
  }
}

void loop() {

  int valor_ldr = analogRead(LDR);
  modo_noturno = valor_ldr < limite_ldr; // Define se está escuro

  if (modo_noturno != modo_noturno_ativo) {
    intervalo = 0; // Reinicia o intervalo
    modo_noturno_ativo = modo_noturno; // modifica o estado do modo noturno ativo

    if (modo_noturno) {
      Serial.println("está escuro, inicializando o modo noturno");
    } else {
      Serial.println("está claro, inicializando o modo semaforo padrão");
      proximo_estado = ESTADO_VERDE; //ao fim do modo noturno, o led retorna para o verde
    }
  }

  // Conta o tempo decorrido desde o inicio do código
  tempo_decorrido = millis();

  // Atualiza os estados do semáforo ao passar o tempo do intervalo
  if (tempo_decorrido - tempo_anterior >= intervalo) {

    // Atualiza o tempo anterior
    tempo_anterior = tempo_decorrido;

    // Desliga todos os LEDs antes de mudar de estado
    resetarLeds();

    if (modo_noturno_ativo) {
      modoNoturno(); // Executa o comportamento do modo noturno
    } else {
      alternarSemaforo(); // Alterna entre os outros estados do semáforo
    }
  }

  if (proximo_estado == ESTADO_VERDE && !modo_noturno_ativo) { // verifica se está com o led vermelho ligado, ou seja o próximo estado é o verde e se não está de noite
    estado_botao = digitalRead(BOTAO);

    if ((millis() - ultimo_tempo_debounce) > intervalo_debounce) {
      // verifica se o estado do botão está pressionado
      if (estado_botao == HIGH) {
        contagem_botao += 1;
        Serial.println("Botao apertado");
        if (!botao_apertado) {
          botao_apertado = true;
          intervalo = 1000; // define o intervalo como 1 segundo
          tempo_anterior = tempo_decorrido; // define que o tempo anterior é igual o tempo decorrido, para o intervalo ser contado a partir de agora
        }

        if (contagem_botao == 3) {
          Serial.println("Alerta");
          int httpResponseCode = http.POST("ALERTA"); // publica a informação http
          if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
          }
          else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
          }
        }
      }
      ultimo_tempo_debounce = millis();
    }
  }
}

// Desliga todos os LEDs
void resetarLeds() {
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERDE, LOW);
}

// alterna entre os estados do semaforo
void alternarSemaforo() {
  switch (proximo_estado) {
    case ESTADO_VERMELHO:
      estadoVermelho();
      break;
    case ESTADO_AMARELO:
      estadoAmarelo();
      break;
    case ESTADO_VERDE:
      estadoVerde();
      break;
  }
}

// Liga o LED vermelho e ajusta o intervalo
void estadoVermelho() {
  digitalWrite(LED_VERMELHO, HIGH);
  intervalo = 5000;
  proximo_estado = ESTADO_VERDE; // Próximo estado
}

// Liga o LED amarelo e ajusta o intervalo
void estadoAmarelo() {
  digitalWrite(LED_AMARELO, HIGH);
  intervalo = 2000;
  proximo_estado = ESTADO_VERMELHO; // Próximo estado
}

// Liga o LED verde e ajusta o intervalo
void estadoVerde() {
  digitalWrite(LED_VERDE, HIGH);
  botao_apertado = false; // reinicia o botao, como sem estar apertado
  contagem_botao = 0; // reinicia a contagem do botao
  intervalo = 3000;
  proximo_estado = ESTADO_AMARELO; // Próximo estado
}

// Alterna o LED amarelo para o modo noturno
void modoNoturno() {
  static bool led_amarelo_ligado = false;
  if (led_amarelo_ligado) {
    digitalWrite(LED_AMARELO, HIGH);
  }
  led_amarelo_ligado = !led_amarelo_ligado; // Alterna o estado do LED
  intervalo = 1000; // alterna o led a cada 1 segundo
}