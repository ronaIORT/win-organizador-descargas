# organizadorCarpetaDownloads (version en C)
Un organizador de archivos de alto rendimiento diseñado para Windows 10/11, enfocado en el **en el minimo consumo de recursos** y la ejecucion en segundo plano.

## ¿Por que en C?
A diferencia de las soluciones basados en scripts de alto nivel, esta implementacion utiliza la API nativa de Windows. Esto permite:
- **Consumo de RAM insignificante**: Ideal para equipos con recursos limitados.
- **Eficiencia de CPU**: El programa permanece en estado de espera (idling) y solo se activa cuando el sistema de archivos notifica cambios.
- **Compatibilidad nativa**: Utiliza las capacidades del sistema operativo para una integracion fluida.
- **Cero dependencias**: No requiere entornos de ejecucion externos.

## Caracteristicas
- **Monitoreo en tiempo real**: Detecta archivos nuevos al instante.
- **Clasificacion inteligente**: Mueve archivos a carpetas correspondientes basadas en su extension.
- **Robustez**: Maneja conflictos de nombres añadiendo sufijos numericos automaticamente.
- **Seguridad**: Ignora archivos temporales de descarga (.crdownload, .tmp) para evitar mover archivos que aun no estan completamente descargados.

## Categorias
El organizador clasifica los archivos en las siguientes categorias:

| Categoria | Extensiones |
|-----------|-------------|
| Imagenes | .jpg, .jpeg, .png, .gif, .bmp, .svg, .webp, .ico |
| PDFs | .pdf |
| Texto | .txt, .md |
| Documentos | .doc, .docx, .rtf, .odt |
| HojasCalculo | .xls, .xlsx, .csv, .ods |
| Presentaciones | .ppt, .pptx, .odp |
| Videos | .mp4, .avi, .mkv, .mov, .wmv, .flv, .webm |
| Audio | .mp3, .wav, .flac, .aac, .ogg, .m4a, .wma |
| Comprimidos | .zip, .rar, .7z, .tar, .gz, .bz2 |
| Ejecutables | .exe, .msi, .bat, .cmd |
| Codigo | .py, .js, .html, .css, .java, .cpp, .c, .h, .php, .json, .xml |
| Torrents | .torrent |
| Otros | Cualquier otra extension |

## Estructura de carpetas
Los archivos se organizan dentro de `Downloads/Organizado/`:

```
Downloads/
└── Organizado/
    ├── Imagenes/
    ├── PDFs/
    ├── Texto/
    ├── Documentos/
    ├── HojasCalculo/
    ├── Presentaciones/
    ├── Videos/
    ├── Audio/
    ├── Comprimidos/
    ├── Ejecutables/
    ├── Codigo/
    ├── Torrents/
    └── Otros/
```

## Como detenerlo
Presiona `Ctrl+C` en la consola para detener el programa de forma segura.

## Compilacion
La forma mas simple de tener tu .exe es:
1. Instalar MinGW-w64
2. Ejecutar en la terminal:
   ```bash
   gcc organizadorCarpetaDownloads.c -o organizador.exe -lshlwapi
   ```
3. El archivo `organizador.exe` se generara en el mismo directorio.
4. Hacer doble clic en `organizador.exe` para ejecutarlo.
