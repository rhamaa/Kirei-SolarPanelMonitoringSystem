#include "InfluxConfig.h"

#include <HTTPClient.h>
#include <WiFi.h>

#include <cstdio>
#include <cstring>

namespace InfluxModule {

namespace {

bool hasValue(const char* value) {
  return value != nullptr && value[0] != '\0';
}

}  // namespace

bool InfluxClient::begin(const Config& config) {
  config_ = config;
  status_ = Status::Idle;
  lastHttpCode_ = 0;
  initialized_ = true;
  return isConfigValid();
}

bool InfluxClient::writeLineProtocol(const char* lineProtocol) {
  if (!initialized_ || !config_.enabled) {
    status_ = Status::BadConfig;
    return false;
  }

  if (!hasValue(lineProtocol)) {
    status_ = Status::WriteFailed;
    return false;
  }

  if (!isConfigValid()) {
    status_ = Status::BadConfig;
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    status_ = Status::NoWifi;
    return false;
  }

  char writeUrl[256] = {};
  if (!buildWriteUrl(writeUrl, sizeof(writeUrl))) {
    status_ = Status::BadConfig;
    return false;
  }

  char authHeader[160] = {};
  std::snprintf(authHeader, sizeof(authHeader), "Token %s", config_.token);

  HTTPClient http;
  http.setTimeout(config_.timeoutMs);

  if (!http.begin(writeUrl)) {
    status_ = Status::HttpError;
    return false;
  }

  http.addHeader("Authorization", authHeader);
  http.addHeader("Content-Type", "text/plain; charset=utf-8");
  http.addHeader("Accept", "application/json");

  const int httpCode = http.POST(reinterpret_cast<uint8_t*>(const_cast<char*>(lineProtocol)),
                                 std::strlen(lineProtocol));
  lastHttpCode_ = httpCode;
  http.end();

  // InfluxDB v2 write returns 204 No Content on success.
  const bool ok = (httpCode == 204) || (httpCode == 200);
  status_ = ok ? Status::Ok : Status::HttpError;
  return ok;
}

Status InfluxClient::getStatus() const {
  return status_;
}

int InfluxClient::getLastHttpCode() const {
  return lastHttpCode_;
}

Config InfluxClient::getConfig() const {
  return config_;
}

void InfluxClient::printStatus(Print& out) const {
  out.printf("[INFLUX] Enabled     : %s\n", config_.enabled ? "true" : "false");
  out.printf("[INFLUX] URL         : %s\n", hasValue(config_.url) ? config_.url : "-");
  out.printf("[INFLUX] Org         : %s\n", hasValue(config_.org) ? config_.org : "-");
  out.printf("[INFLUX] Bucket      : %s\n", hasValue(config_.bucket) ? config_.bucket : "-");
  out.printf("[INFLUX] Token       : %s\n", hasValue(config_.token) ? "(set)" : "-");
  out.printf("[INFLUX] Measurement : %s\n",
             hasValue(config_.measurement) ? config_.measurement : "-");
  out.printf("[INFLUX] Status      : %s\n", statusToString(status_));
  out.printf("[INFLUX] Last HTTP   : %d\n", lastHttpCode_);
}

const char* InfluxClient::statusToString(Status status) {
  switch (status) {
    case Status::Idle:
      return "Idle";
    case Status::NoWifi:
      return "No WiFi";
    case Status::Ok:
      return "OK";
    case Status::HttpError:
      return "HTTP Error";
    case Status::BadConfig:
      return "Bad Config";
    case Status::WriteFailed:
      return "Write Failed";
    default:
      return "Unknown";
  }
}

bool InfluxClient::isConfigValid() const {
  return hasValue(config_.url) && hasValue(config_.org) && hasValue(config_.bucket) &&
         hasValue(config_.token);
}

bool InfluxClient::buildWriteUrl(char* buffer, size_t bufferSize) const {
  if (buffer == nullptr || bufferSize == 0U) {
    return false;
  }

  const char* precision = hasValue(config_.precision) ? config_.precision : kDefaultPrecision;

  const int written = std::snprintf(buffer,
                                    bufferSize,
                                    "%s/api/v2/write?org=%s&bucket=%s&precision=%s",
                                    config_.url,
                                    config_.org,
                                    config_.bucket,
                                    precision);

  return written > 0 && written < static_cast<int>(bufferSize);
}

}  // namespace InfluxModule
