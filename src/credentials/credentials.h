#pragma once

char *get_credentials_filepath(const char *appname);
int save_credentials(const char *filepath, const char *credentials, const char *password);
char *load_credentials(const char *filepath, const char *password);
