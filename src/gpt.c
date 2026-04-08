/*
 * weathercli - terminal weather forecast application
 * Written by xir. (github.com/xernot)
 * Copyright (C) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "gpt.h"
#include "../vendor/cJSON.h"
#include "config.h"
#include "http.h"
#include "secrets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int gpt_is_available(void) { return secrets_get(OPENAI_API_KEY_ENV) != NULL; }

static void append_weather_data(const forecast_t *forecast, int day_start,
                                int day_end, char *buf, size_t buf_size,
                                int offset) {
  int end = day_end;
  if (end > forecast->num_days)
    end = forecast->num_days;

  for (int i = day_start; i < end && offset < (int)buf_size - 128; i++) {
    const day_weather_t *d = &forecast->days[i];
    offset +=
        snprintf(buf + offset, buf_size - offset,
                 "%s: %s, High %.0fC, Low %.0fC, "
                 "Wind %.0fkm/h, Precip %.1fmm, Humidity %d%%\n",
                 d->date, weather_code_to_string(d->weather_code), d->temp_max,
                 d->temp_min, d->wind_speed_max, d->precipitation, d->humidity);
  }
}

static void append_all_weather(const forecast_t *forecast, char *buf,
                               size_t buf_size, int offset) {
  append_weather_data(forecast, 0, forecast->num_days, buf, buf_size, offset);
}

static cJSON *build_request_body(const char *prompt) {
  cJSON *body = cJSON_CreateObject();
  cJSON_AddStringToObject(body, "model", GPT_MODEL);
  cJSON_AddNumberToObject(body, "max_completion_tokens", GPT_MAX_TOKENS);
  cJSON_AddNumberToObject(body, "temperature", GPT_TEMPERATURE);

  cJSON *messages = cJSON_AddArrayToObject(body, "messages");
  cJSON *msg = cJSON_CreateObject();
  cJSON_AddStringToObject(msg, "role", "user");
  cJSON_AddStringToObject(msg, "content", prompt);
  cJSON_AddItemToArray(messages, msg);

  return body;
}

static int extract_gpt_response(const char *response_data, char *output,
                                size_t output_size) {
  cJSON *json = cJSON_Parse(response_data);
  if (!json) {
    snprintf(output, output_size, GPT_ERR_INVALID_JSON);
    return -1;
  }

  cJSON *error = cJSON_GetObjectItem(json, "error");
  if (error) {
    cJSON *msg = cJSON_GetObjectItem(error, "message");
    if (msg && msg->valuestring) {
      snprintf(output, output_size, GPT_ERR_API_ERROR_FMT, msg->valuestring);
    }
    cJSON_Delete(json);
    return -1;
  }

  cJSON *choices = cJSON_GetObjectItem(json, "choices");
  cJSON *first = cJSON_GetArrayItem(choices, 0);
  cJSON *message = cJSON_GetObjectItem(first, "message");
  cJSON *content = cJSON_GetObjectItem(message, "content");

  if (content && content->valuestring) {
    snprintf(output, output_size, "%s", content->valuestring);
    cJSON_Delete(json);
    return 0;
  }

  snprintf(output, output_size, GPT_ERR_UNEXPECTED_FORMAT);
  cJSON_Delete(json);
  return -1;
}

static int send_to_gpt(const char *prompt, char *output, size_t output_size) {
  const char *api_key = secrets_get(OPENAI_API_KEY_ENV);
  if (!api_key) {
    snprintf(output, output_size, GPT_ERR_NO_KEY_FMT, OPENAI_API_KEY_ENV);
    return -1;
  }

  cJSON *body = build_request_body(prompt);
  char *body_str = cJSON_PrintUnformatted(body);
  cJSON_Delete(body);

  char auth[GPT_AUTH_HEADER_MAX];
  snprintf(auth, sizeof(auth), "Authorization: Bearer %s", api_key);

  http_buffer_t response;
  http_buffer_init(&response);

  int result = http_post_json(OPENAI_API_URL, body_str, auth, &response);
  free(body_str);

  if (result != 0) {
    http_buffer_free(&response);
    snprintf(output, output_size, GPT_ERR_REQUEST_FAILED);
    return -1;
  }

  result = extract_gpt_response(response.data, output, output_size);
  http_buffer_free(&response);
  return result;
}

int gpt_summarize(const forecast_t *forecast, const char *location_name,
                  const char *activity_prompt, int day_start, int day_end,
                  char *summary, size_t summary_size) {
  char prompt[GPT_PROMPT_MAX];
  int offset =
      snprintf(prompt, sizeof(prompt),
               "Summarize this weather forecast for %s in 2-3 sentences. "
               "Be concise and conversational. %s\n\n",
               location_name, activity_prompt);

  append_weather_data(forecast, day_start, day_end, prompt, sizeof(prompt),
                      offset);

  return send_to_gpt(prompt, summary, summary_size);
}

int gpt_format_forecast(const forecast_t *forecast, const char *location_name,
                        char *buf, size_t buf_size, int offset) {
  offset +=
      snprintf(buf + offset, buf_size - offset, "--- %s ---\n", location_name);
  append_all_weather(forecast, buf, buf_size, offset);
  return strlen(buf);
}

int gpt_chat(const char *weather_context, const char *question, char *response,
             size_t response_size) {
  char prompt[GPT_CHAT_PROMPT_MAX];
  int offset =
      snprintf(prompt, sizeof(prompt),
               "You are a weather assistant. Here is forecast data for "
               "the relevant locations:\n\n%s\n"
               "User question: %s\n\n"
               "Answer concisely based on the forecast data above. "
               "If the user asks about a route or trip between locations, "
               "compare the weather at each location.",
               weather_context, question);
  (void)offset;

  return send_to_gpt(prompt, response, response_size);
}
