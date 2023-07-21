/**
 * @file curl_send_message.h
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
#ifndef MFEXTENSIONS_DESTINATIONS_DETAIL_CURL_SEND_MESSAGE_H_
#define MFEXTENSIONS_DESTINATIONS_DETAIL_CURL_SEND_MESSAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * \file curl_send_message.h
 * This file wraps the C-language cURL SMTP functions
 * Code is from https://curl.haxx.se/libcurl/c/example.html
 */

/**
 * \brief Structure to track progress of upload in cURL send function
 */
struct upload_status
{
	size_t pos;           ///< Current position within payload
	size_t size;          ///< Size of payload
	const char* payload;  ///< payload string
};

/**
 * \brief Sends a message to the given SMTP server
 * \param dest URL of SMTP server, in form smtp://[HOST]:[PORT]
 * \param to Array of strings containing destination addresses
 * \param to_size Size of the to array (must be >0!)
 * \param from Address that the email is originating from
 * \param payload Message payload, including RFC5322 headers
 * \param payload_size Size of the message payload, in bytes
 */
void send_message(const char* dest, const char* to[], size_t to_size, const char* from, const char* payload,
                  size_t payload_size);

/**
 * \brief Sends a message to the given SMTP server, using SSL encryption
 * \param dest URL of SMTP server, in form smtps://[HOST]:[PORT]
 * \param to Array of strings containing destination addresses
 * \param to_size Size of the to array (must be >0!)
 * \param from Address that the email is originating from
 * \param payload Message payload, including RFC5322 headers
 * \param payload_size Size of the message payload, in bytes
 * \param username Credentials for logging in to SMTPS server
 * \param pw  Credentials for logging in to SMTPS server (Recommend empty string)
 * \param disableVerify Disable verification of host certificate (Recommend 0)
 */
void send_message_ssl(const char* dest, const char* to[], size_t to_size, const char* from, const char* payload,
                      size_t payload_size, const char* username, const char* pw, int disableVerify);

#ifdef __cplusplus
}
#endif

#endif  // MFEXTENSIONS_DESTINATIONS_DETAIL_CURL_SEND_MESSAGE_H_
