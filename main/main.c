

#include <stdio.h>
#include "esp_wifi.h"  // Funções do driver de Wi-Fi
#include "esp_wifi_types_generic.h"
#include "nvs_flash.h" // Funções da NVS (área da flash que guarda dados e sobrevive ao reset)
#include "esp_netif.h" // Camada de interface de rede (a pilha TCP/IP)
#include "esp_event.h" // Sistema de eventos deo ESP-IDF
#include "esp_err.h"   // Tipo esp_err_t e a macro ESP_ERROR_CHECK
#include "esp_log.h"   // ESP_LOGI / ESP_LOGW para mensagens no monitor
#include "mqtt_client.h" // API do cliente MQTT do ESP-IDF (componente esp-mqtt)
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "dht.h"

static const char* WIFI_TAG = "TAG = WIFI";
static const char* MQTT_TAG = "TAG = MQTT";

#define PIN_PIR GPIO_NUM_27
#define PIN_DHT GPIO_NUM_26

#define PIN_LED_TEMP GPIO_NUM_32
#define PIN_LED_UMID GPIO_NUM_33
#define PIN_LED_PIR GPIO_NUM_25

// Configurações de conexão
#define WIFI_SSID "SSID"	// WIFI SSID
#define WIFI_PASS "PASSWORD"	// WIFI PASSWORD

#define MQTT_URI "mqtt://IP_ADDRESS:PORT" 		// protocolo + endereço + porta do broker
#define MQTT_USER "USERNAME" 				// usuário para identificação
#define MQTT_PASS "USER_PASSW" 				// senha da autenticação
#define MQTT_CLIENT "esp_iot_id" 			// identificador único do seu ESP32 no broker

// Topicos de leitura do ESP32
#define TOPIC_LEDtemp "/ESP32/TOPIC/temp"			// Define o tópico para o LED
#define TOPIC_LEDumid "/ESP32/TOPIC/umid"			// Define o tópico para o LED
#define TOPIC_LEDpir "/ESP32/TOPIC/presenca"			// Define o tópico para o LED

// Tópicos de publicação do ESP32
#define TOPIC_TEMP "/ESP32/TOPIC/temperatura"		// Define o tópico para a temperatura
#define TOPIC_UMID "/ESP32/TOPIC/umidade"			// Define o tópico para a umidade
#define TOPIC_PIR "/ESP32/TOPIC/presenca"			// Define o tópico para o PIR

static esp_mqtt_client_handle_t mqtt_client = NULL; // "identificador" do cliente; fica global para usarmos depois (publicar etc.)
	    
static volatile bool mqtt_conectado = false;	// true somente enquanto o estiver conectado ao broker

static bool topico_igual (esp_mqtt_event_handle_t event, const char* topico) {	// Retorna se o tópico do evento é igual ao tópico dado como parâmetro
    return (event->topic_len == strlen(topico)) && (strncmp(event->topic, topico, event->topic_len) == 0);
}

// Função chamada pelo ESP-IDF a cada evento do cliente MQTT
static void mqtt_event_handler(void *handler_args,	// Argumento extra do registro (não usamos)
				esp_event_base_t base,	// Família do evento (aqui, sempre MQTT)
				int32_t event_id,	// qual evento aconteceu
				void *event_data) {	// dados do evento

    esp_mqtt_event_handle_t event = event_data; // Converte os dados genéricos para o tipo de evento MQTT
	
    if (event_id == MQTT_EVENT_CONNECTED) {						// Conectou ao broker
	mqtt_conectado = true;
	ESP_LOGI(MQTT_TAG, "Conectado ao broker.");					// Avisa no monitor

	esp_mqtt_client_subscribe(mqtt_client, TOPIC_LEDtemp, 0);				// increve-se no tópico da temperatura
	esp_mqtt_client_subscribe(mqtt_client, TOPIC_LEDumid, 0);				// increve-se no tópico da umidade
	esp_mqtt_client_subscribe(mqtt_client, TOPIC_LEDpir, 0);				// increve-se no tópico do pir

	esp_mqtt_client_publish(mqtt_client, TOPIC_TEMP, "ligado", 0, 0, 0);	// publica mensagem inicial
	esp_mqtt_client_publish(mqtt_client, TOPIC_UMID, "ligado", 0, 0, 0);	// publica mensagem inicial
	esp_mqtt_client_publish(mqtt_client, TOPIC_PIR, "ligado", 0, 0, 0);	// publica mensagem inicial

    } else if (event_id == MQTT_EVENT_DISCONNECTED) {		// Perdeu a conexão com o broker
	mqtt_conectado = false;
	ESP_LOGW(MQTT_TAG, "Desconectado do broker.");		// Só avisa no monitor (o cliente tenta reconectar sozinho)
    
    } else if (event_id == MQTT_EVENT_DATA) {				// Chegou uma mensagem de um tópico inscrito
	printf("Topico: %.*s\n", event->topic_len, event->topic);	// Mostra o tópico
	printf("Dados: %.*s\n", event->data_len, event->data);		// Mostra a mensagem

	if (topico_igual(event, TOPIC_LEDtemp)) {
	    gpio_set_level(PIN_LED_TEMP, ((event->data[0] == '1') ? 1 : 0)); 	// Liga/Desliga o LED TEMP

	} else if (topico_igual(event, TOPIC_LEDumid)) {
	    gpio_set_level(PIN_LED_UMID, ((event->data[0] == '1') ? 1 : 0));	// Liga/Desliga o LED UMID

	} else if (topico_igual(event, TOPIC_LEDpir)) {
	    gpio_set_level(PIN_LED_PIR, ((event->data[0] == '1') ? 1 : 0));	// Liga/Desliga o LED PIR

	}
    } else if (event_id == MQTT_EVENT_ERROR) {		// Algo deu errado
	ESP_LOGE(MQTT_TAG, "Erro no cliente MQTT");	// Log de erro
    }

    // Qualquer outra coisa cai aqui e não faz nada.
}

static void mqtt_app_init(void) {				// Função que monta o cliente
    esp_mqtt_client_config_t mqtt_cfg = {			// Estrutura com todos os parâmetros do cliente
	.broker.address.uri = MQTT_URI,				// onde está o broker
	.credentials.username = MQTT_USER,			// usuário
	.credentials.client_id = MQTT_CLIENT,			// id do cliente
	.credentials.authentication.password = MQTT_PASS,	// senha
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);		// Cria o cliente com essa configuração e devolve o identificador 

    // Liga o handler a todos os eventos do cliente; Sem isso, nenhum evento chega nele
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

// Função chamada pelo ESP-IDF toda vez que um evento registrado acontece
static void wifi_event_handler(void *arg,			// Argumento extra que passamos no registro (não usamos)
				esp_event_base_t event_base,	// "família" do evento: WIFI_EVENT ou IP_EVENT
				int32_t event_id,		// Qual evento específico dessa família
				void *event_data) {		// Dados do evento (ex.: o IP obtido)
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
	esp_wifi_connect();					//O wifi ligou, tenta conectaro ao SSID configurado
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
	ESP_LOGW(WIFI_TAG, "Desconectado. Tentando de novo...");	// Avisa que a conexão falhou ou caiu
	esp_wifi_connect();						// Tenta conectar novamente
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
	ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data; 	// Converte os dados genéricos para o tipo "IP obtido"
	ESP_LOGI(WIFI_TAG, "Conectado! IP: " IPSTR, IP2STR(&event->ip_info.ip)); // Mostra o IP no formato x.x.x.x
	
	if (mqtt_client == NULL) {		// Só entra na primeira vez que o wifi ganha ip
	    mqtt_app_init();			// Cria o cliente MQTT e registra o handler
	    esp_mqtt_client_start(mqtt_client);	// Inicia o cliente: conecta ao broker em segundo plano
	}
    }
}

void leitura_pir(void *parametros) {
    while (1) {
        int presenca = gpio_get_level(PIN_PIR);
        printf("Presenca: %d\n", presenca);

	char pirStr[10];
	snprintf(pirStr, sizeof(pirStr), "%d", presenca);  

	esp_mqtt_client_publish(mqtt_client, TOPIC_PIR, pirStr, 0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void leitura_dht(void *parametros) {
    float temperatura, umidade;
    
    while (1) {
        esp_err_t resultado = dht_read_float_data(
            DHT_TYPE_DHT11,
            PIN_DHT,
            &umidade,
            &temperatura
        );

        if (resultado == ESP_OK) {

            printf("Temperatura: %.2f C\n", temperatura);
            printf("Umidade: %.2f %%\n", umidade);
	    
	    char tempStr[10];
	    char umidStr[10];

	    snprintf(tempStr, sizeof(tempStr), "%.2f", temperatura);
	    snprintf(umidStr, sizeof(umidStr), "%.2f", umidade);

	    esp_mqtt_client_publish(mqtt_client, TOPIC_TEMP, tempStr, 0, 0, 0);	// publica dado
	    esp_mqtt_client_publish(mqtt_client, TOPIC_UMID, umidStr, 0, 0, 0);	// publica dado

        } else {

            printf("Erro na leitura do DHT11!\n");
        }

        printf("------------------------\n");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    // GPIOs configuration
    gpio_reset_pin(PIN_PIR);
    gpio_set_direction(PIN_PIR, GPIO_MODE_INPUT);

    gpio_reset_pin(PIN_DHT);
    gpio_set_pull_mode(PIN_DHT, GPIO_PULLUP_ONLY);

    gpio_set_direction(PIN_LED_TEMP, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LED_UMID, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LED_PIR, GPIO_MODE_OUTPUT);

    xTaskCreate(
        leitura_pir,
        "leitura_pir",
        2048,
        NULL,
        5,
        NULL
    );

    xTaskCreate(
        leitura_dht,
        "leitura_dht",
        4096,
        NULL,
        5,
        NULL
    );

    esp_err_t ret = nvs_flash_init(); // Tenta inicializar a NVS e guarda o código de retorno
    // Se a NVS estiver cheia ou veio de uma versão antifa do IDF, ela não inicia
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	ESP_ERROR_CHECK(nvs_flash_erase()); // Apaga a partição NVS inteira
	ret = nvs_flash_init();		    // Tenta inicializar de novo, agora limpa
    }
    ESP_ERROR_CHECK(ret); // Se ainda houver erro, interrompe a execução aqui e exibe o erro
    
    ESP_ERROR_CHECK(esp_netif_init()); // Inicializa a pilha TCP/IP (IP, DHCP, Sockets)
    
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Cria a fila padrão que entrega os eventos do wifi
    

    esp_netif_create_default_wifi_sta(); // Cria a interface em modo de estação (cliente) e já prepara o DHCP para receber IP
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // Monta a configuração padrão do driver (buffers, tarefas internas etc)
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));		 // Inicializa o driver com essa configuração
    
    wifi_config_t wifi_config = {   // Estrutura com os dados da rede
	.sta = {		    // Bloco de configuração do modo estação
	    .ssid = WIFI_SSID,      // Nome da rede que queremos acessar
	    .password = WIFI_PASS,  // Senha da rede que queremos acessar
				    
	    // Ajuste ao modo de autenticação da sua rede. Uma dessas duas pode funcionar, se não, você deve pesquisar a correta para você
	    .threshold.authmode = WIFI_AUTH_WPA_PSK, // Só aceita redes com mínima segurança
	    //.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK, // Só aceita redes com mínima segurança
	},
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));			// Define o modo: estação (cliente), não ponto de acesso
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));	// Entrega o SSID e a SENHA ao driver

    // Registra o handler para eventos de Wi-Fi (iniciou, desconectou...), qualquer  id
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    
    // Registra o mesmo handler, só para o evento "ganhei um IP"
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_start()); // Liga o wifi (dispara WIFI_EVENT_STA_START, que o handler já está esperando)
    
}
