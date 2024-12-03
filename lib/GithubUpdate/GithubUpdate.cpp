#include "GithubUpdate.h"

const char *github_comRootCACert = "-----BEGIN CERTIFICATE-----\n"
                                   "MIICjzCCAhWgAwIBAgIQXIuZxVqUxdJxVt7NiYDMJjAKBggqhkjOPQQDAzCBiDEL\n"
                                   "MAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNl\n"
                                   "eSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMT\n"
                                   "JVVTRVJUcnVzdCBFQ0MgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAwMjAx\n"
                                   "MDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNVBAgT\n"
                                   "Ck5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVUaGUg\n"
                                   "VVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBFQ0MgQ2VydGlm\n"
                                   "aWNhdGlvbiBBdXRob3JpdHkwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAQarFRaqflo\n"
                                   "I+d61SRvU8Za2EurxtW20eZzca7dnNYMYf3boIkDuAUU7FfO7l0/4iGzzvfUinng\n"
                                   "o4N+LZfQYcTxmdwlkWOrfzCjtHDix6EznPO/LlxTsV+zfTJ/ijTjeXmjQjBAMB0G\n"
                                   "A1UdDgQWBBQ64QmG1M8ZwpZ2dEl23OA1xmNjmjAOBgNVHQ8BAf8EBAMCAQYwDwYD\n"
                                   "VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAwNoADBlAjA2Z6EWCNzklwBBHU6+4WMB\n"
                                   "zzuqQhFkoJ2UOQIReVx7Hfpkue4WQrO/isIJxOzksU0CMQDpKmFHjFJKS04YcPbW\n"
                                   "RNZu9YO6bVi9JNlWSOrvxKJGgYhqOkbRqZtNyWHa0V1Xahg=\n"
                                   "-----END CERTIFICATE-----\n";

const char *github_ioRootCACert = "-----BEGIN CERTIFICATE-----\n"
                                  "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
                                  "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
                                  "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
                                  "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"
                                  "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
                                  "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"
                                  "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"
                                  "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"
                                  "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"
                                  "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"
                                  "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"
                                  "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"
                                  "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"
                                  "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"
                                  "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"
                                  "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"
                                  "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"
                                  "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"
                                  "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
                                  "MrY=\n"
                                  "-----END CERTIFICATE-----\n";

GithubUpdate::GithubUpdate()
{
    _prefs.begin("github_update");
}

GithubUpdate::~GithubUpdate()
{
    _prefs.end();
}

void GithubUpdate::update(String url, String lastModified)
{
    if (url.startsWith("https://github.com"))
    {
        _client.setCACert(github_comRootCACert);
    }
    else
    {
        _client.setCACert(github_ioRootCACert);
    }

    _https.setReuse(true);

    const char *headerKeys[] = {"Last-Modified", "Content-Length", "Content-Type"};
    const size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
    _https.collectHeaders(headerKeys, headerKeysCount);

    _https.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

    log_d("[HTTPS] %s", url.c_str());
    if (!_https.begin(_client, url))
    {
        log_d("[HTTPS] Unable to connect");
        return;
    }

    _https.addHeader("If-Modified-Since", lastModified);

    log_d("[HTTPS] GET...");
    // start connection and send HTTP header
    int httpCode = _https.GET();

    // httpCode will be negative on error
    if (httpCode <= 0)
    {
        log_d("[HTTPS] GET... failed, error: %s", _https.errorToString(httpCode).c_str());
        char *buf = new char[100];
        _client.lastError(buf, sizeof buf);
        log_d("[HTTPS] %s\n", buf);
        return;
    }

    // HTTP header has been send and Server response header has been handled
    log_d("[HTTPS] GET... code: %d", httpCode);

    if (httpCode == HTTP_CODE_FOUND)
    {
        log_d("Followed redirect");
        String nextUrl = _https.getLocation();
        log_d("Location: %s", nextUrl.c_str());
        if (nextUrl.startsWith("https://github.com"))
        {
            log_d("  ...to same domain");
            update(nextUrl, lastModified);
        }
        else
        {
            log_d("  ...to different domain");
            // update(nextUrl, lastModified);
            _client.setCACert(github_ioRootCACert);
            update(nextUrl, lastModified);
        }
    }
    else if (httpCode == HTTP_CODE_NOT_MODIFIED)
    {
        log_d("Not modified");
        return;
    }
    else if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
    {
        log_d("Location: %s", _https.getLocation().c_str());
        int count = _https.headers();
        log_d("Headers %d", count);
        for (int i = 0; i < count; i++)
        {
            log_d("%s: %s", _https.headerName(i).c_str(), _https.header(i).c_str());
        }

        long contentLength = atol(_https.header("Content-Length").c_str());
        String lastModified = _https.header("Last-Modified");

        if (!Update.begin(contentLength))
        {
            log_d("Cannot update, not enough space...");
            return;
        }

        log_d("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
        log_d("Last-Modified: %s", lastModified.c_str());
        // No activity would appear on the Serial monitor
        // So be patient. This may take 2 - 5mins to complete
        size_t written = Update.writeStream(_https.getStream());

        if (written != contentLength)
        {
            log_d("Written only : %d/%d. Retry?", written, contentLength);
            return;
        }

        log_d("Written : %d successfully", written);

        if (!Update.end())
        {
            log_d("Error Occurred. Error #: %d", Update.getError());
            return;
        }

        log_d("OTA done!");

        if (!Update.isFinished())
        {
            log_d("Update not finished? Something went wrong!");
            return;
        }

        _prefs.putString("Last-Modified", lastModified);
        _prefs.end();

        log_d("Update successfully completed. Rebooting.");
        ESP.restart();
    }

    _https.end();
}

void GithubUpdate::update(const char *org, const char *repo, const char *asset)
{
    String url = String("https://github.com/");
    url.concat(org);
    url.concat('/');
    url.concat(repo);
    url.concat("/releases/latest/download/");
    url.concat(asset);

    String lastModified = _prefs.getString("Last-Modified");
    update(url, lastModified);
}