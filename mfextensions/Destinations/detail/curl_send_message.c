#include "curl_send_message.h"
#include <curl/curl.h>
#include <time.h>

static size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp)
{
	struct upload_status* upload_ctx = (struct upload_status*)userp;
	size_t rdsize = size * nmemb;

	if ((size == 0) || (nmemb == 0) || (rdsize < 1) || (upload_ctx->pos >= upload_ctx->size))
	{
		return 0;
	}

	if (rdsize + upload_ctx->pos > upload_ctx->size)
	{
		rdsize = upload_ctx->size - upload_ctx->pos;
	}

	memcpy(ptr, &upload_ctx->payload[upload_ctx->pos], rdsize);
	upload_ctx->pos += rdsize;

	return rdsize;
}

void send_message(const char* dest, const char* to[], size_t to_size, const char* from, const char* payload,
                  size_t payload_size)
{
	CURL* curl;
	CURLcode res;
	struct curl_slist* recipients = NULL;
	struct upload_status upload_ctx;

	upload_ctx.pos = 0;
	upload_ctx.size = payload_size;
	upload_ctx.payload = payload;

	curl = curl_easy_init();
	if (curl)
	{
		/* This is the URL for your mailserver */
		curl_easy_setopt(curl, CURLOPT_URL, dest);

		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);

		/* Note that the CURLOPT_MAIL_RCPT takes a list, not a char array.  */

		for (size_t ii = 0; ii < to_size; ++ii)
		{
			recipients = curl_slist_append(recipients, to[ii]);
		}
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		/* send the message (including headers) */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		/* free the list of recipients */
		curl_slist_free_all(recipients);

		curl_easy_cleanup(curl);
	}
}

void send_message_ssl(const char* dest, const char* to[], size_t to_size, const char* from, const char* payload,
                      size_t payload_size, const char* username, const char* pw, int disableVerify)
{
	CURL* curl;
	CURLcode res = CURLE_OK;
	struct curl_slist* recipients = NULL;
	struct upload_status upload_ctx;

	upload_ctx.pos = 0;
	upload_ctx.size = payload_size;
	upload_ctx.payload = payload;

	curl = curl_easy_init();
	if (curl)
	{
		/* Set username and password */
		curl_easy_setopt(curl, CURLOPT_USERNAME, username);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, pw);

		/* This is the URL for your mailserver. Note the use of port 587 here,
		 * instead of the normal SMTP port (25). Port 587 is commonly used for
		 * secure mail submission (see RFC4403), but you should use whatever
		 * matches your server configuration. */
		curl_easy_setopt(curl, CURLOPT_URL, dest);

		/* In this example, we'll start with a plain text connection, and upgrade
		 * to Transport Layer Security (TLS) using the STARTTLS command. Be careful
		 * of using CURLUSESSL_TRY here, because if TLS upgrade fails, the transfer
		 * will continue anyway - see the security discussion in the libcurl
		 * tutorial for more details. */
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

		if (disableVerify)
		{
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		}

		/* Note that this option isn't strictly required, omitting it will result
		 * in libcurl sending the MAIL FROM command with empty sender data. All
		 * autoresponses should have an empty reverse-path, and should be directed
		 * to the address in the reverse-path which triggered them. Otherwise,
		 * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
		 * details.
		 */
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);

		for (size_t ii = 0; ii < to_size; ++ii)
		{
			recipients = curl_slist_append(recipients, to[ii]);
		}
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		/* We're using a callback function to specify the payload (the headers and
		 * body of the message). You could just use the CURLOPT_READDATA option to
		 * specify a FILE pointer to read from. */
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		/* Since the traffic will be encrypted, it is very useful to turn on debug
		 * information within libcurl to see what is happening during the transfer.
		 */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		/* Send the message */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		/* Free the list of recipients */
		curl_slist_free_all(recipients);

		/* Always cleanup */
		curl_easy_cleanup(curl);
	}
}
