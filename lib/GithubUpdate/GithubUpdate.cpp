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
                                  "MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB\n"
                                  "iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n"
                                  "cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n"
                                  "BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw\n"
                                  "MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV\n"
                                  "BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU\n"
                                  "aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy\n"
                                  "dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK\n"
                                  "AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B\n"
                                  "3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY\n"
                                  "tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/\n"
                                  "Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2\n"
                                  "VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT\n"
                                  "79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6\n"
                                  "c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT\n"
                                  "Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l\n"
                                  "c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee\n"
                                  "UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE\n"
                                  "Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd\n"
                                  "BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G\n"
                                  "A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF\n"
                                  "Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO\n"
                                  "VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3\n"
                                  "ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs\n"
                                  "8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR\n"
                                  "iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze\n"
                                  "Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ\n"
                                  "XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/\n"
                                  "qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB\n"
                                  "VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB\n"
                                  "L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG\n"
                                  "jjxDah2nGN59PRbxYvnKkKj9\n"
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