/*=============================================================================
 * Author: Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * Date: 2021/09/20
 *===========================================================================*/

/*==================[inclusiones]============================================*/
#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"

#include "FreeRTOSConfig.h"
#include "keys.h"
/*==================[definiciones y macros]==================================*/
#define RATE 1000
#define LED_RATE pdMS_TO_TICKS(RATE)
/*==================[definiciones de datos internos]=========================*/
const gpioMap_t leds_t[]={LEDB};
const gpioMap_t gpio_t[]={GPIO7};
/*==================[definiciones de datos externos]=========================*/
DEBUG_PRINT_ENABLE;
/*==================[declaraciones de funciones externas]====================*/
extern void keys_Init( void );
extern TickType_t get_diff( uint32_t index );
extern void clear_diff( uint32_t index );
extern uint32_t keys_get_key_count(void);

/*==================[declaraciones de funciones internas]====================*/
void gpio_Init(uint32_t led_count);
void tarea_led( void* taskParmPtr );
void vApplicationMallocFailedHook(void);

/*==================[funcion principal]======================================*/

int main( void )
{
    // ---------- CONFIGURACIONES ------------------------------
    boardConfig();				// Inicializar y configurar la plataforma

    debugPrintConfigUart( UART_USB, 115200 );	// UART for debug messages
    printf( "Ejercicio C_7.\r\n" );

    BaseType_t res;
    uint32_t i;
    uint32_t led_count = keys_get_key_count();

    gpio_Init(led_count); 			//Inicialización de GPIOs

    // Se crean una cantidad de tareas igual a led_count, cada una con un parámetro i distinto
    for ( i = 0 ; i < led_count ; i++ )
    {
        res = xTaskCreate(
                  tarea_led,                     // Funcion de la tarea a ejecutar
                  ( const char * )"tarea_led",   // Nombre de la tarea como String amigable para el usuario
                  configMINIMAL_STACK_SIZE*2, // Cantidad de stack de la tarea
                  i,                          // Parametros de tarea
                  tskIDLE_PRIORITY+1,         // Prioridad de la tarea
                  0                           // Puntero a la tarea creada en el sistema
            );

        // Gestion de errores
        configASSERT( res == pdPASS );
    }

    /* inicializo driver de teclas */
    keys_Init(); //Dentro crea una sola tarea para consultar todas la teclas

    // Iniciar scheduler
    vTaskStartScheduler();					// Enciende tick | Crea idle y pone en ready | Evalua las tareas creadas | Prioridad mas alta pasa a running

    // ---------- REPETIR POR SIEMPRE --------------------------
    configASSERT( 0 );

    // NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa se ejecuta
    // directamenteno sobre un microcontroladore y no es llamado por ningun
    // Sistema Operativo, como en el caso de un programa para PC.
    return TRUE;
}

/*==================[definiciones de funciones internas]=====================*/
void gpio_Init(uint32_t led_count)
{
    uint32_t i;
    for (i = 0; i < led_count; i++)
    {
        gpioInit(gpio_t[i], GPIO_OUTPUT);
    }
}

void tarea_led( void* taskParmPtr )
{
    uint32_t index = ( uint32_t ) taskParmPtr;

    // ---------- CONFIGURACIONES ------------------------------
    TickType_t dif=KEYS_INVALID_TIME;
    TickType_t xPeriodicity =  LED_RATE;		// Tarea periodica cada 1000 ms
    TickType_t xLastWakeTime = xTaskGetTickCount(); //Inicialización. Luego es actualizado automaticamnte dentro de vTaskDelayUntil
    // ---------- REPETIR POR SIEMPRE --------------------------
    while( TRUE )
    {
        dif = get_diff( index );

        if (dif != KEYS_INVALID_TIME && dif != 0)
        {
            gpioWrite(leds_t[index], ON);
            gpioWrite(gpio_t[index], ON);
            if (dif < LED_RATE)
            {
                vTaskDelay(dif);
                gpioWrite(leds_t[index], OFF);
                gpioWrite(gpio_t[index], OFF);
            }
        }
        vTaskDelayUntil( &xLastWakeTime, xPeriodicity );  //xLastWakeTime se actualiza automaticamente
    }
}

/* hook que se ejecuta si al necesitar un objeto dinamico, no hay memoria disponible */
void vApplicationMallocFailedHook()
{
    printf( "Malloc Failed Hook!\n" );
    configASSERT( 0 );
}
/*==================[fin del archivo]========================================*/
