/*=============================================================================
 * Author: Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * Date: 2021/09/20
 *===========================================================================*/

/*==================[ Inclusions ]============================================*/
#include "keys.h"

#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"

/*=====[Definition macros of private constants]==============================*/
#define DEBOUNCE_TIME   40
#define DEBOUNCE_TIME_MS pdMS_TO_TICKS(DEBOUNCE_TIME)
#define KEY_COUNT   sizeof(keys_config)/sizeof(keys_config[0])

/*=====[ Definitions of private data types ]===================================*/

/*=====[Definitions of private global variables]=============================*/
static const t_key_config  keys_config[] = { TEC1, TEC2, TEC3,TEC4 };
static t_key_data keys_data[KEY_COUNT];

/*=====[Prototypes (declarations) of private functions]======================*/
static void keys_ButtonError( uint32_t index );
static void buttonPressed( uint32_t index );
static void buttonReleased( uint32_t index );
static void task_tecla( void* taskParmPtr );

/*=====[Definitions of public global variables]==============================*/

/*=====[prototype of private functions]=================================*/

/*=====[Implementations of public functions]=================================*/
uint32_t keys_get_key_count(void)
{
    uint32_t led_count;

    led_count = sizeof(keys_config)/sizeof(keys_config[0]);

    return led_count;
}

TickType_t get_diff( uint32_t index )
{
    TickType_t tiempo;

    tiempo = keys_data[index].time_diff;

    return tiempo;
}

void clear_diff( uint32_t index )
{
    keys_data[index].time_diff = KEYS_INVALID_TIME;
}

void keys_Init( void )
{
    BaseType_t res;
    uint32_t i;

    for ( i = 0 ; i < KEY_COUNT ; i++ )
    {
        keys_data[i].state          = BUTTON_UP;  // Set initial state
        keys_data[i].time_down      = KEYS_INVALID_TIME;
        keys_data[i].time_up        = KEYS_INVALID_TIME;
        keys_data[i].time_diff      = KEYS_INVALID_TIME;
    }
    // Crear tareas en freeRTOS
    res = xTaskCreate (
              task_tecla,					// Funcion de la tarea a ejecutar
              ( const char * )"task_tecla",	// Nombre de la tarea como String amigable para el usuario
              configMINIMAL_STACK_SIZE*2,	// Cantidad de stack de la tarea
              0,							// Parametros de tarea
              tskIDLE_PRIORITY+1,			// Prioridad de la tarea
              0							    // Puntero a la tarea creada en el sistema
        );

    // Gestión de errores
    configASSERT( res == pdPASS );
}

// keys_ Update State Function
void keys_Update( uint32_t index )
{
    switch( keys_data[index].state )
    {
        case STATE_BUTTON_UP:
            /* CHECK TRANSITION CONDITIONS */
            if( !gpioRead( keys_config[index].tecla ) )
            {
                keys_data[index].state = STATE_BUTTON_FALLING;
            }
            break;

        case STATE_BUTTON_FALLING:
            /* ENTRY */

            /* CHECK TRANSITION CONDITIONS */
            if( !gpioRead( keys_config[index].tecla ) )
            {
                keys_data[index].state = STATE_BUTTON_DOWN;

                /* ACCION DEL EVENTO !*/
                buttonPressed( index );
            }
            else
            {
                keys_data[index].state = STATE_BUTTON_UP;
            }

            /* LEAVE */
            break;

        case STATE_BUTTON_DOWN:
            /* CHECK TRANSITION CONDITIONS */
            if( gpioRead( keys_config[index].tecla ) )
            {
                keys_data[index].state = STATE_BUTTON_RISING;
            }
            break;

        case STATE_BUTTON_RISING:
            /* ENTRY */

            /* CHECK TRANSITION CONDITIONS */

            if( gpioRead( keys_config[index].tecla ) )
            {
                keys_data[index].state = STATE_BUTTON_UP;

                /* ACCION DEL EVENTO ! */
                buttonReleased( index );
            }
            else
            {
                keys_data[index].state = STATE_BUTTON_DOWN;
            }

            /* LEAVE */
            break;

        default:
            keys_ButtonError( index );
            break;
    }
}

/*=====[Implementations of private functions]================================*/

/* accion de el evento de tecla pulsada */
static void buttonPressed( uint32_t index )
{
    TickType_t current_tick_count = xTaskGetTickCount();

    keys_data[index].time_down = current_tick_count;
}

/* accion de el evento de tecla liberada */
static void buttonReleased( uint32_t index )
{
    TickType_t current_tick_count = xTaskGetTickCount();

    keys_data[index].time_up    = current_tick_count;
    keys_data[index].time_diff  = keys_data[index].time_up - keys_data[index].time_down;
}

static void keys_ButtonError( uint32_t index )
{
    keys_data[index].state = BUTTON_UP;
}

/**
 * @brief Consulta por todas las teclas y bloquea la tarea por un tiempo de DEBOUNCE_TIME_MS
 *
 * @param taskParmPtr
 */
void task_tecla( void* taskParmPtr )
{
    uint32_t i;
    while( TRUE )
    {
        for ( i = 0 ; i < KEY_COUNT ; i++ )
        {
            keys_Update( i );
        }
        vTaskDelay( DEBOUNCE_TIME_MS );
    }
}
