#include "sdcard.h"

__attribute__((unused)) static const char *TAG = "SDcard";


esp_err_t sdcard_initialize(esp_vfs_fat_sdmmc_mount_config_t *_mount_config, sdmmc_card_t *_sdcard,
                            sdmmc_host_t *_host, spi_bus_config_t *_bus_config, sdspi_device_config_t *_slot_config)
{
    esp_err_t err_code;
    ESP_LOGI(__func__, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI(__func__, "Using SPI peripheral");

    err_code = spi_bus_initialize(_host->slot, _bus_config, SPI_DMA_CHAN);
    if (err_code != ESP_OK)
    {
        ESP_LOGE(__func__, "Failed to initialize bus.");
        ESP_LOGE(__func__, "Failed to initialize the SDcard.");
        return ESP_ERROR_SD_INIT_FAILED;
    }
    _slot_config->gpio_cs = CONFIG_PIN_NUM_CS;
    _slot_config->host_id = _host->slot;

    ESP_LOGI(__func__, "Mounting filesystem");
    err_code = esp_vfs_fat_sdspi_mount(mount_point, _host, _slot_config, _mount_config, &_sdcard);
    
    if (err_code != ESP_OK) {
        if (err_code == ESP_FAIL) {
            ESP_LOGE(__func__, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(__func__, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(err_code));
        }
        return err_code;
    }
    ESP_LOGI(__func__, "SDCard has been initialized.");
    ESP_LOGI(__func__, "Filesystem mounted");

    // Card has been initialized, print its properties
    ESP_LOGI(__func__, "SDCard properties.");
    sdmmc_card_print_info(stdout, _sdcard);
    return ESP_OK;
}


esp_err_t sdcard_writeDataToFile(const char *nameFile, const char *format, ...)
{
    char pathFile[64];
    sprintf(pathFile, "%s/%s.txt", mount_point, nameFile);

    ESP_LOGI(__func__, "Opening file %s...", pathFile);
    FILE *file = fopen(pathFile, "a+");
    if (file == NULL)
    {
        ESP_LOGE(__func__, "Failed to open file for writing.");
        return ESP_ERROR_SD_OPEN_FILE_FAILED;
    }
    
    char *dataString;
    int lenght;
    va_list argumentsList;
    va_list argumentsList_copy;
    va_start(argumentsList, format);
    va_copy(argumentsList_copy, argumentsList);
    lenght = vsnprintf(NULL, 0, format, argumentsList_copy);
    va_end(argumentsList_copy);

    dataString = (char*)malloc(++lenght);
    if(dataString == NULL) {
        ESP_LOGE(TAG, "Failed to create string data for writing.");
        va_end(argumentsList);
        return ESP_FAIL;
    }

    vsnprintf(dataString, (++lenght), format, argumentsList);
    ESP_LOGI(TAG, "Success to create string data(%d) for writing.", lenght);
    ESP_LOGI(TAG, "Writing data to file %s...", pathFile);
    ESP_LOGI(TAG, "%s;\n", dataString);

    int returnValue = 0;
    returnValue = fprintf(file, "%s", dataString);
    if (returnValue < 0)
    {
        ESP_LOGE(__func__, "Failed to write data to file %s.", pathFile);
        return ESP_ERROR_SD_WRITE_DATA_FAILED;
    }
    ESP_LOGI(__func__, "Success to write data to file %s.", pathFile);
    fclose(file);
    va_end(argumentsList);
    free(dataString);
    return ESP_OK;
}


esp_err_t sdcard_readDataFromFile(const char *nameFile, const char *format, ...)
{
    char pathFile[64];
    sprintf(pathFile, "%s/%s.txt", mount_point, nameFile);

    ESP_LOGI(__func__, "Opening file %s...", pathFile);
    FILE *file = fopen(pathFile, "r");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for reading.");
        return ESP_ERROR_SD_OPEN_FILE_FAILED;
    }

    // Read a string data from file
    char dataStr[256];
    char *returnPtr;
    returnPtr = fgets(dataStr, sizeof(dataStr), file);
    fclose(file);
    
    if (returnPtr == NULL)
    {
        ESP_LOGE(__func__, "Failed to read data from file %s.", pathFile);
        return ESP_ERROR_SD_READ_DATA_FAILED;
    }

    va_list argumentsList;
    va_start(argumentsList, format);
    int returnValue = 0;
    returnValue = vsscanf(dataStr, format, argumentsList);
    va_end(argumentsList);

    if (returnValue < 0)
    {
        ESP_LOGE(__func__, "Failed to read data from file %s.", pathFile);
        return ESP_ERROR_SD_READ_DATA_FAILED;
    }
    
    return ESP_OK;
}

esp_err_t sdcard_renameFile(const char *oldNameFile, char *newNameFile)
{
    // Check if destination file exists before renaming
    struct stat st;
    char _oldNameFile[64];
    char _newNameFile[64];
    sprintf(_oldNameFile, "%s/%s.txt", MOUNT_POINT, oldNameFile);
    sprintf(_newNameFile, "%s/%s.txt", MOUNT_POINT, newNameFile);
    ESP_LOGI(__func__, "Update file name from %s to %s", _oldNameFile, _newNameFile);

    if (stat(_newNameFile, &st) == 0) {
        ESP_LOGE(__func__, "File \"%s\" exists.", _newNameFile);
        return ESP_ERROR_SD_RENAME_FILE_FAILED;
    }

    // Rename original file
    ESP_LOGI(__func__, "Renaming file %s to %s", _oldNameFile, _newNameFile);
    if (rename(_oldNameFile, _newNameFile) != 0) 
    {
        ESP_LOGE(__func__, "Rename failed");
        return ESP_ERROR_SD_RENAME_FILE_FAILED;
    } else {
        ESP_LOGI(__func__, "Rename successful");
        return ESP_OK;
    }
}

esp_err_t sdcard_removeFile(const char *nameFile)
{
    struct stat st;
    char _nameFile[64];
    sprintf(_nameFile, "%s/%s.txt", MOUNT_POINT, nameFile);
    
    // Check whether destination file exists or not
    if (stat(_nameFile, &st) != 0) {
        ESP_LOGE(__func__, "File \"%s\" doesn't exists.", _nameFile);
        return ESP_ERROR_SD_REMOVE_FILE_FAILED;
    }

    if (remove(_nameFile) != 0)     
    {
        ESP_LOGE(__func__, "Remove failed");
        return ESP_ERROR_SD_REMOVE_FILE_FAILED;
    } else {
        ESP_LOGW(__func__, "Remove successful");
        return ESP_OK;
    }

}


esp_err_t sdcard_deiDdeviceIDitialize(const char* _mount_point, sdmmc_card_t *_sdcard, sdmmc_host_t *_host)
{
    ESP_LOGI(__func__, "DeiDdeviceIDitializing SD card...");
    // Unmount partition and disable SPI peripheral.
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdcard_unmount(_mount_point, _sdcard));
    ESP_LOGI(__func__, "Card unmounted.");

    //deiDdeviceIDitialize the bus after all devices are removed
    ESP_ERROR_CHECK_WITHOUT_ABORT(spi_bus_free(_host->slot));
    return ESP_OK;
}


esp_err_t sdcard_createFolder(const char *nameFolder)
{
    char _nameFolder[64];
    sprintf(_nameFolder, "%s/%s", MOUNT_POINT, nameFolder);
    ESP_LOGI(__func__, "Creating folder: %s", _nameFolder);

    //check if Folder exists or not
    struct stat st;
    if (stat(_nameFolder, &st) == 0)
    {
        ESP_LOGE(__func__, "Folder \"%s\" already exists.", _nameFolder);
        return ESP_ERROR_SD_CREAT_FOLDER_FAILED;
    }

    // 7: The owner has permission to RWX (read, write, execute)
    // 7: Group has permission to RWX (read, write, execute)
    // 7: Other users have permission to RWX (read, write, execute)
    // 0777: alow everyone to have access to the new folder
    if (mkdir(_nameFolder, 0777) != 0) 
    {
        ESP_LOGE(__func__, "Failed to creat Folder");
        return ESP_ERROR_SD_CREAT_FOLDER_FAILED;
    } else {
        ESP_LOGI(__func__, "Folder create successfully");
        return ESP_OK;
    }
}



esp_err_t sdcard_getFilePath(const char *fileName)
{
    char filePath[128]; 

    snprintf(filePath, sizeof(filePath), "%s/%s", MOUNT_POINT, fileName);

    struct stat st;
    if (stat(filePath, &st) == 0)
    {
        ESP_LOGI(__func__, "File \"%s\" found at path: %s", fileName, filePath);
        return ESP_OK;
    }
    else
    {
        ESP_LOGE(__func__, "File \"%s\" not found.", fileName);
        return ESP_ERROR_SD_GET_FILEPATH_FAILED;
    }
}



esp_err_t sdcard_createFile(const char *fileName)
{
    char _fileName[128];
    sprintf(_fileName, "%s/%s", MOUNT_POINT, fileName);
    ESP_LOGI(__func__, "Creating file: %s", _fileName);

    // check if file exists or not
    struct stat st;
    if (stat(_fileName, &st) == 0)
    {
        ESP_LOGE(__func__, "File \"%s\" already exists.", _fileName);
        return ESP_ERROR_SD_CREATE_FILE_FAILED;
    }

    // creat new file to write (w)
    FILE *file = fopen(_fileName, "w");
    if (file == NULL)
    {
        ESP_LOGE(__func__, "Failed to create file");
        return ESP_ERROR_SD_CREATE_FILE_FAILED;
    }
    
    // close file
    fclose(file);

    ESP_LOGI(__func__, "File created successfully");
    return ESP_OK;
}



esp_err_t sdcard_createDeviceStructure(const char *deviceID) 
    {
        #define MAX_PATH_LENGTH 100
        if (strlen(deviceID) >= MAX_PATH_LENGTH) {
            //Make sure deviceID does not exceed the maximum size for the path
            return ESP_ERROR_SD_CREATE_DEVICE_STRUCTURE;
        }

        char firmwarePath[MAX_PATH_LENGTH];
        char timestampPath[MAX_PATH_LENGTH];
        char systemPath[MAX_PATH_LENGTH];
        char firmwareJsonPath[MAX_PATH_LENGTH];
        char firmwareBinPath[MAX_PATH_LENGTH];
        char systemJsonPath[MAX_PATH_LENGTH];
        char logPath[MAX_PATH_LENGTH];
        char dataPath[MAX_PATH_LENGTH];

        //Creat folder structure
    
        snprintf(firmwarePath, sizeof(firmwarePath), "%s/Firmware", deviceID);
        snprintf(timestampPath, sizeof(timestampPath), "%s/Timestamp", deviceID);
        snprintf(systemPath, sizeof(systemPath), "%s/Timestamp/system", deviceID);

        if (sdcard_createFolder(firmwarePath) != ESP_OK ||
            sdcard_createFolder(timestampPath) != ESP_OK ||
            sdcard_createFolder(systemPath) != ESP_OK) 
        {
            return ESP_ERROR_SD_CREATE_DEVICE_STRUCTURE; 
        }

        //Creat file
        if (snprintf(dataPath, sizeof(dataPath), "%s/data.csv", timestampPath) >= sizeof(dataPath) ||
            snprintf(firmwareJsonPath, sizeof(firmwareJsonPath), "%s/firmware.json", firmwarePath) >= sizeof(firmwareJsonPath) ||
            snprintf(firmwareBinPath, sizeof(firmwareBinPath), "%s/firmwareold.bin", firmwarePath) >= sizeof(firmwareBinPath) ||
            snprintf(systemJsonPath, sizeof(systemJsonPath), "%s/system.json", systemPath) >= sizeof(systemJsonPath) ||
            snprintf(logPath, sizeof(logPath), "%s/Log.log", systemPath) >= sizeof(logPath)) 
        {
            return ESP_ERROR_SD_CREATE_DEVICE_STRUCTURE; 
        }

        if (sdcard_createFile(firmwareJsonPath) != ESP_OK ||
            sdcard_createFile(firmwareBinPath) != ESP_OK ||
            sdcard_createFile(systemJsonPath) != ESP_OK ||
            sdcard_createFile(logPath) != ESP_OK ||
            sdcard_createFile(dataPath) != ESP_OK) 
        {
            return ESP_ERROR_SD_CREATE_DEVICE_STRUCTURE;
        }

        return ESP_OK;
    }