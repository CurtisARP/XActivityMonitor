#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

int http_client_init(void);
void http_client_cleanup(void);
void flush_to_api(int force);

#endif
