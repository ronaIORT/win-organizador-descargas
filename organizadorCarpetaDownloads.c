// Organizador de descargas para Windows 10 - Versión completa
// Compilar con: gcc organizador.c -o organizador.exe -lshlwapi

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlwapi.h>   // PathCombine, PathFindExtension

// Configuración
#define MAX_RETRIES 10
#define RETRY_DELAY_MS 500

// Estructura para categorías
typedef struct {
    const wchar_t* category;
    const wchar_t* exts[20];
} FileCategory;

// Definición de categorías (Modificado: añadido PDFs y Texto, quitados de Documentos)
static const FileCategory categories[] = {
    { L"Imagenes",       { L".jpg", L".jpeg", L".png", L".gif", L".bmp", L".svg", L".webp", L".ico", NULL } },
    { L"PDFs",           { L".pdf", NULL } }, // Nueva categoría
    { L"Texto",          { L".txt", L".md", NULL } }, // Nueva categoría
    { L"Documentos",     { L".doc", L".docx", L".rtf", L".odt", NULL } }, // Modificado (sin .pdf ni .txt)
    { L"HojasCalculo",   { L".xls", L".xlsx", L".csv", L".ods", NULL } },
    { L"Presentaciones", { L".ppt", L".pptx", L".odp", NULL } },
    { L"Videos",         { L".mp4", L".avi", L".mkv", L".mov", L".wmv", L".flv", L".webm", NULL } },
    { L"Audio",          { L".mp3", L".wav", L".flac", L".aac", L".ogg", L".m4a", L".wma", NULL } },
    { L"Comprimidos",    { L".zip", L".rar", L".7z", L".tar", L".gz", L".bz2", NULL } },
    { L"Ejecutables",    { L".exe", L".msi", L".bat", L".cmd", NULL } },
    { L"Codigo",         { L".py", L".js", L".html", L".css", L".java", L".cpp", L".c", L".h", L".php", L".json", L".xml", NULL } },
    { L"Torrents",       { L".torrent", NULL } },
    { L"Otros",          { NULL } }
};

// Extensiones temporales a ignorar
static const wchar_t* temp_exts[] = { L".tmp", L".crdownload", L".part", L".download", L".opdownload", NULL };

// Variables globales para el monitoreo
HANDLE hStopEvent = NULL;
int fRunning = 1;

// Obtener ruta de Downloads usando variable de entorno (más compatible)
BOOL GetDownloadsPath(wchar_t* path, DWORD size) {
    // Intentar con %USERPROFILE%\Downloads
    DWORD len = GetEnvironmentVariableW(L"USERPROFILE", path, size);
    if (len > 0 && len < size - 12) { // dejar espacio para "\Downloads"
        wcscat(path, L"\\Downloads");
        return TRUE;
    }
    // Fallback: C:\Users\Public\Downloads
    wcscpy(path, L"C:\\Users\\Public\\Downloads");
    return TRUE;
}

// Determinar categoría por extensión
const wchar_t* GetCategoryForExtension(const wchar_t* ext) {
    for (int i = 0; i < sizeof(categories)/sizeof(categories[0]); i++) {
        const FileCategory* cat = &categories[i];
        for (int j = 0; cat->exts[j] != NULL; j++) {
            if (_wcsicmp(ext, cat->exts[j]) == 0) {
                return cat->category;
            }
        }
    }
    return L"Otros";
}

// Verificar si es extensión temporal
BOOL IsTempExtension(const wchar_t* ext) {
    for (int i = 0; temp_exts[i] != NULL; i++) {
        if (_wcsicmp(ext, temp_exts[i]) == 0)
            return TRUE;
    }
    return FALSE;
}

// Generar nombre único en destino (usa wsprintfW)
void GetUniqueDestPath(const wchar_t* destDir, const wchar_t* filename, wchar_t* outPath, DWORD outSize) {
    wchar_t base[MAX_PATH], ext[MAX_PATH];
    wchar_t candidate[MAX_PATH];

    wcscpy(base, filename);
    wchar_t* dot = wcsrchr(base, L'.');
    if (dot) {
        wcscpy(ext, dot);
        *dot = L'\0';
    } else {
        ext[0] = L'\0';
    }

    int counter = 1;
    while (1) {
        if (counter == 1) {
            wsprintfW(candidate, L"%s\\%s%s", destDir, base, ext);
        } else {
            wsprintfW(candidate, L"%s\\%s_%d%s", destDir, base, counter, ext);
        }
        if (GetFileAttributesW(candidate) == INVALID_FILE_ATTRIBUTES) {
            wcscpy(outPath, candidate);
            return;
        }
        counter++;
    }
}

// Mover archivo si está listo (no bloqueado)
BOOL MoveFileIfReady(const wchar_t* srcPath, const wchar_t* filename) {
    DWORD attr = GetFileAttributesW(srcPath);
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY))
        return FALSE;

    const wchar_t* ext = PathFindExtensionW(srcPath);
    if (IsTempExtension(ext)) {
        // wprintf(L"Ignorado temporal: %s\n", filename); // opcional
        return FALSE;
    }

    // Verificar si el archivo está bloqueado (intentando abrir exclusivo)
    HANDLE hFile = CreateFileW(srcPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;  // Bloqueado
    }
    CloseHandle(hFile);

    const wchar_t* category = GetCategoryForExtension(ext);

    wchar_t downloadsPath[MAX_PATH];
    GetDownloadsPath(downloadsPath, MAX_PATH);

    wchar_t destDir[MAX_PATH];
    wsprintfW(destDir, L"%s\\Organizado\\%s", downloadsPath, category);

    CreateDirectoryW(destDir, NULL);

    wchar_t destPath[MAX_PATH];
    GetUniqueDestPath(destDir, filename, destPath, MAX_PATH);

    if (MoveFileExW(srcPath, destPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) {
        wprintf(L"[OK] Movido: %s -> %s\\\n", filename, category);

        // Notificar al Explorador de Windows que el archivo original ya no existe
        SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, srcPath, NULL);

        // Notificar que se creó el nuevo archivo en la carpeta destino (opcional)
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, destPath, NULL);

        return TRUE;

    } else {
        wprintf(L"[ERROR] Error moviendo %s (error %lu)\n", filename, GetLastError());
        return FALSE;
    }
}

// Función para organizar archivos existentes al inicio
void OrganizeExistingFiles() {
    wchar_t downloadsPath[MAX_PATH];
    if (!GetDownloadsPath(downloadsPath, MAX_PATH)) {
        wprintf(L"No se pudo obtener la ruta de Downloads.\n");
        return;
    }

    wchar_t searchPath[MAX_PATH];
    wsprintfW(searchPath, L"%s\\*", downloadsPath);

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        wprintf(L"No se pudo leer la carpeta Downloads.\n");
        return;
    }

    wprintf(L"Organizando archivos existentes...\n");
    int count = 0;
    do {
        // Ignorar . y ..
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;

        // Ignorar directorios (incluyendo la carpeta Organizado)
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        // Construir ruta completa
        wchar_t srcPath[MAX_PATH];
        PathCombineW(srcPath, downloadsPath, findData.cFileName);

        // Evitar archivos temporales por extensión
        const wchar_t* ext = PathFindExtensionW(srcPath);
        if (IsTempExtension(ext))
            continue;

        // Mover el archivo (no reintentos porque ya está completo)
        if (MoveFileIfReady(srcPath, findData.cFileName)) {
            count++;
        }
    } while (FindNextFileW(hFind, &findData) != 0);

    FindClose(hFind);
    wprintf(L"Archivos existentes organizados: %d movidos.\n", count);
}

// Hilo de monitoreo (igual que antes)
DWORD WINAPI MonitorThread(LPVOID lpParam) {
    wchar_t watchPath[MAX_PATH];
    GetDownloadsPath(watchPath, MAX_PATH);

    HANDLE hDir = CreateFileW(
        watchPath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        wprintf(L"Error abriendo directorio %s (error %lu)\n", watchPath, GetLastError());
        return 1;
    }

    BYTE buffer[65536];
    DWORD bytesReturned;
    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    wprintf(L"Vigilando: %s\n", watchPath);
    wprintf(L"Destino: %s\\Organizado\n", watchPath);
    wprintf(L"Presiona Ctrl+C para detener\n\n");

    while (fRunning) {
        BOOL success = ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            &overlapped,
            NULL
        );

        if (!success) {
            wprintf(L"Error en ReadDirectoryChangesW (error %lu)\n", GetLastError());
            break;
        }

        HANDLE events[2] = { overlapped.hEvent, hStopEvent };
        DWORD waitRes = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (waitRes == WAIT_OBJECT_0 + 1) break;
        if (waitRes != WAIT_OBJECT_0) break;

        GetOverlappedResult(hDir, &overlapped, &bytesReturned, FALSE);

        FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)buffer;
        while (1) {
            if (event->Action == FILE_ACTION_ADDED || event->Action == FILE_ACTION_MODIFIED) {
                wchar_t filename[MAX_PATH];
                int nameLen = event->FileNameLength / sizeof(wchar_t);
                if (nameLen < MAX_PATH) {
                    wcsncpy(filename, event->FileName, nameLen);
                    filename[nameLen] = L'\0';

                    wchar_t srcPath[MAX_PATH];
                    PathCombineW(srcPath, watchPath, filename);

                    // Evitar archivos dentro de Organizado
                    if (wcsstr(srcPath, L"\\Organizado\\") != NULL) {
                        // Saltar
                    } else {
                        for (int retry = 0; retry < MAX_RETRIES; retry++) {
                            if (MoveFileIfReady(srcPath, filename)) break;
                            Sleep(RETRY_DELAY_MS);
                        }
                    }
                }
            }
            if (event->NextEntryOffset == 0) break;
            event = (FILE_NOTIFY_INFORMATION*)((BYTE*)event + event->NextEntryOffset);
        }
        ResetEvent(overlapped.hEvent);
    }

    CloseHandle(hDir);
    CloseHandle(overlapped.hEvent);
    return 0;
}

// Manejador de consola (Ctrl+C)
BOOL WINAPI ConsoleHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT || dwCtrlType == CTRL_CLOSE_EVENT) {
        wprintf(L"\nDeteniendo organizador...\n");
        fRunning = 0;
        if (hStopEvent) SetEvent(hStopEvent);
        return TRUE;
    }
    return FALSE;
}

int main() {
    // Configurar consola para UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Crear directorio Organizado si no existe
    wchar_t downloadsPath[MAX_PATH];
    GetDownloadsPath(downloadsPath, MAX_PATH);
    wchar_t destRoot[MAX_PATH];
    wsprintfW(destRoot, L"%s\\Organizado", downloadsPath);
    CreateDirectoryW(destRoot, NULL);

    // PASO 1: Organizar archivos existentes
    OrganizeExistingFiles();

    // PASO 2: Iniciar monitoreo en tiempo real
    hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    HANDLE hThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    CloseHandle(hStopEvent);
    wprintf(L"Organizador finalizado.\n");
    return 0;
}
