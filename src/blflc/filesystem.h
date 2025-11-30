#ifndef _BLFLCFILESYSTEM
#define _BLFLCFILESYSTEM

#include <Arduino.h>

// Config file path
extern const char *configPath;

// Generate a random string of given length
char *generateRandomString(int length);

// Save current configuration to filesystem
void saveFileSystem();

// Load configuration from filesystem
void loadFileSystem();

// Delete configuration file
void deleteFileSystem();

// Check if configuration file exists
bool hasFileSystem();

// Initialize the filesystem
void setupFileSystem();

#endif
