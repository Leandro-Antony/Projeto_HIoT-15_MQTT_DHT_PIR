#include <stdio.h>

#include "mqtt_client.h" // Componente para implementação do protocolo MQTT
#include "esp_log.h" // Componente para ESP_LOG

// Configurações do broker
#define MQTT_BROKER_URI "mqtt://IP_DO_BROKER:PORTA"
#define MQTT_USER "USER"
#define MQTT_PASSWORD "PASSWORD"

// TAG pro MQTT
static const char* TAG_MQTT = "TAG = MQTT";

// O Handle é uma referência ao cliente MQTT.
// Para que a gente acesse as funções do cliente por ele
esp_mqtt_client_handle_t client = NULL;

void mqtt_init(void) {
    // Estrutura que armazena as configurações do cliente MQTT.
    esp_mqtt_client_config_t mqtt_cfg = {
	//Endereço e porta do broker
	.broker.address.uri = MQTT_BROKER_URI,

	//Credenciais de autenticaçãoo do broker
	.credentials.username = MQTT_USER,
	.credentials.authentication.password = MQTT_PASSWORD
    };

    //Cria e inicializa uma instância do cliente MQTT
    //A estruturamqtt_cfg é utilizada com configuração.
    //
    //O retorno é um handle que permite maipular esse cliente.
    client = esp_mqtt_client_init(&mqtt_cfg);
}
