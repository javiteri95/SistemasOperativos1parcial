typedef struct informacion_cliente{
    char* ruta_archivo_fuente;
    char* ruta_archivo_ejecutable;
    int tiempo_envio;
    char* respuesta;
    int orden_llegada;
    int identificador_usuario;

    char* banderas;
}informacion_cliente;