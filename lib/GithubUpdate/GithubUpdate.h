#ifndef __GITHUB_UPDATE_H_
#define __GITHUB_UPDATE_H_

#include <Arduino.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WifiClientSecure.h>
#include <Preferences.h>

class GithubUpdate
{
protected:
    WiFiClientSecure _client;
    HTTPClient _https;
    Preferences _prefs;

    void update(String url, String lastModified);

public:
    GithubUpdate();
    ~GithubUpdate();
    void update(const char *org, const char *repo, const char *asset = "firmware.bin");
};

#endif