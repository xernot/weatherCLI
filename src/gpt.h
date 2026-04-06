#ifndef GPT_H
#define GPT_H

#include "weather.h"

#include <stddef.h>

/* Maximum length of a GPT summary */
#define GPT_SUMMARY_MAX 2048

/* Generate a GPT summary of the forecast for a specific day range and activity.
   Returns 0 on success, -1 on error. */
int gpt_summarize(const Forecast *forecast, const char *location_name,
                  const char *activity_prompt, int day_start, int day_end,
                  char *summary, size_t summary_size);

/* Send a free-form chat question with weather context to GPT.
   weather_context is a pre-built string with forecast data for one or more
   locations. Returns 0 on success, -1 on error. */
int gpt_chat(const char *weather_context, const char *question, char *response,
             size_t response_size);

/* Format a forecast into a text block for use in chat context.
   Appends to buf at the given offset. Returns new offset. */
int gpt_format_forecast(const Forecast *forecast, const char *location_name,
                        char *buf, size_t buf_size, int offset);

/* Check if the OpenAI API key is configured. Returns 1 if available, 0 if not.
 */
int gpt_is_available(void);

#endif /* GPT_H */
