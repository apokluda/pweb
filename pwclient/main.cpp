/*
 *  Copyright (C) 2013 Alexander Pokluda
 *
 *  This file is part of pWeb.
 *
 *  pWeb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pWeb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pWeb.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include "mongoose.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

char HANDLED[1];

static void *callback(enum mg_event event,
                      struct mg_connection *conn) {
  const struct mg_request_info *request_info = mg_get_request_info(conn);

  if (event == MG_NEW_REQUEST) {
    char content[1024];
    int content_length = snprintf(content, sizeof(content),
                                  "Hello from mongoose! Remote port: %d",
                                  request_info->remote_port);
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: %d\r\n"        // Always set Content-Length
              "\r\n"
              "%s",
              content_length, content);
    // Mark as processed
    return HANDLED;
  } else {
    return NULL;
  }
}

int main(int argc, const char** argv)
{
    HANDLED[0] = '\0';

    struct mg_context *ctx;
    const char *options[] = {"listening_ports", "8080", NULL};

    ctx = mg_start(&callback, NULL, options);
    getchar();  // Wait until user hits "enter"
    mg_stop(ctx);

    return 0;
}
