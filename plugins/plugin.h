/*! \file   plugin.h
 * \author Lorenzo Miniero <lorenzo@meetecho.com>
 * \copyright GNU General Public License v3
 * \brief  Plugin-Gateway communication
 * \details  This header contains the definition of the callbacks both
 * the gateway and all the plugins need too implement to interact with
 * each other. The structures to make the communication possible are
 * defined here as well.
 * 
 * In particular, the gateway implements the \c janus_callbacks interface.
 * This means that, as a plugin, you can use the methods it exposes to
 * contact the gateway, e.g., in order to have it relay a message, event
 * or RTP/RTCP packet to the peer you're handling. In particular, the
 * methods the gateway exposes to plugins are:
 * 
 * - \c push_event(): to send a JSON message/event to the peer (with or without
 * an attached JSEP formatted SDP to negotiate a WebRTC PeerConnection);
 * the syntax of the message/event is completely up to you, the only
 * important thing is that it MUST be a JSON object, as it will be included
 * as such within the Janus session/handle protocol;
 * - \c relay_rtp(): to send/relay the peer an RTP packet;
 * - \c relay_rtcp(): to send/relay the peer an RTCP message.
 * - \c relay_data(): to send/relay the peer a SCTP DataChannel message.
 * 
 * On the other hand, a plugin that wants to register at the gateway
 * needs to implement the \c janus_plugin interface. Besides, as a
 * plugin is a shared object, and as such external to the gateway itself,
 * in order to be dynamically loaded at startup it needs to implement
 * the \c create_p() hook as well, that should return a pointer to the
 * plugin instance. This is an example of such a step:
 * 
\verbatim
static janus_plugin myplugin = {
	[..]
};

janus_plugin *create(void) {
	JANUS_LOG(LOG_VERB, , "%s created!\n", MY_PLUGIN_NAME);
	return &myplugin;
}
\endverbatim
 * 
 * This will make sure that your plugin is loaded at startup by the gateway,
 * if it is deployed in the proper folder.
 * 
 * As anticipated and described in the above example, a plugin must basically
 * be an instance of the \c janus_plugin type. As such, it must implement
 * the following methods and callbacks for the gateway:
 * 
 * - \c init(): this is called by the gateway as soon as your plugin is started;
 * this is where you should setup your plugin (e.g., static stuff and reading
 * the configuration file);
 * - \c destroy(): on the other hand, this is called by the gateway when it
 * is shutting down, and your plugin should too;
 * - \c get_api_compatibility(): this method MUST return JANUS_PLUGIN_API_VERSION;
 * - \c get_version(): this method should return a numeric version identifier (e.g., 3);
 * - \c get_version_string(): this method should return a verbose version identifier (e.g., "v1.0.1");
 * - \c get_description(): this method should return a verbose description of your plugin (e.g., "This is my awesome plugin that does this and that");
 * - \c get_name(): this method should return a short display name for your plugin (e.g., "My Awesome Plugin");
 * - \c get_package(): this method should return a unique package identifier for your plugin (e.g., "janus.plugin.myplugin");
 * - \c create_session(): this method is called by the gateway to create a session between you and a peer;
 * - \c handle_message(): a callback to notify you the peer sent you a message/request;
 * - \c setup_media(): a callback to notify you the peer PeerConnection is now ready to be used;
 * - \c incoming_rtp(): a callback to notify you a peer has sent you a RTP packet;
 * - \c incoming_rtcp(): a callback to notify you a peer has sent you a RTCP message;
 * - \c incoming_data(): a callback to notify you a peer has sent you a message on a SCTP DataChannel;
 * - \c hangup_media(): a callback to notify you the peer PeerConnection has been closed (e.g., after a DTLS alert);
 * - \c query_session(): this method is called by the gateway to get plugin-specific info on a session between you and a peer;
 * - \c destroy_session(): this method is called by the gateway to destroy a session between you and a peer.
 * 
 * The gateway \c janus_callbacks interface is provided to a plugin, together
 * with the path to the configurations files folder, in the \c init() method.
 * This path can be used to read and parse a configuration file for the
 * plugin: the plugins we made available out of the box use the package
 * name as a name for the file (e.g., \c janus.plugin.echotest.cfg for
 * the Echo Test plugin), but you're free to use a different one, as long
 * as it doesn't collide with existing ones. Besides, the existing plugins
 * use the same INI format for configuration files the gateway uses (relying
 * on the \c janus_config helpers for the purpose) but again, if you prefer
 * a different format (XML, JSON, etc.) that's up to you. 
 *
 * Both the the gateway and a plugin can have several different sessions
 * with the same and/or different peers: to match a specific session,
 * a plugin can rely on a mapping called janus_plugin_session that
 * is what all the communication between the plugins and the gateway
 * (that is, both methods invoked by the gateway and callbacks invoked by
 * the plugins) will make use of. See the janus_videoroom.c plugin for
 * an example of multiple handles associated to the same peer. 
 * 
 * All messages/requests/events sent to and received from a plugin are
 * asynchronous, meaning there's no way to immediately reply to a message
 * sent by a browser, for instance. Messages/requests coming from browsers
 * in a \c handle_message() callback, though, have a transaction
 * identifier, which you can use in a \c push_event() reply to allow the
 * browser to match it to the original request, if needed.
 * 
 * As anticipated, both \c handle_message() and \c push_event() can attach
 * a JSEP/SDP payload. This means that a browser, for instance, can attach
 * a JSEP/SDP offer to negotiate a WebRTC PeerConnection with a plugin: the plugin
 * would then need to provide, immediately or not, a JSEP/SDP answer to
 * do so. At the same time, a plugin may want to originate the call instead:
 * in that case, the plugin would attach a JSEP/SDP offer in a \c push_event()
 * call, to which the browser would then need to reply with a JSEP/SDP answer,
 * as described in \ref JS.
 * \note It's important to notice that, while the gateway core would indeed
 *  take care of the WebRTC PeerConnection setup itself in terms of
 * ICE/DTLS/RT(C)P on your behalf, plugins are what will actually manipulate
 * the media flowing around, and as such it's them who are responsible for
 * what concerns the codec negotiation in a JSEP/SDP offer/answer. This
 * normally is not something you need to worry about, especially if you're
 * just moving SDP around (e.g., janus_echotest.c or janus_videocall.c). 
 * If your plugin is going to generate media frames (e.g., as janus_audiobridge.c),
 * you only support some codecs (e.g., Opus in janus_audiobridge.c) or you
 * want to use the same SDP offer for several different sessions (e.g., a webinar), 
 * you need to make sure that your offer/answer does not contain anything
 * you don't support. Besides, you also need to make sure that you use
 * SDP-provided information (e.g., payload types) coherently. 
 * 
 * \todo Right now plugins can only interact with peers, through the gateway.
 * Besides, a single PeerConnection can at the moment be used by only one
 * plugin, as that plugin is actually the "owner" of the PeerConnection itself.
 * In next versions of Janus we'll work on stuff like plugins "chaining": that
 * is, plugins that can act as "filters" for other plugins (e.g., transcoders)
 * or as additional sources/sinks for the same PeerConnection of the same peer
 * (e.g., to add recording functionality to a video conference using a
 * different plugin).
 * 
 * \ingroup pluginapi
 * \ref pluginapi
 */

#ifndef _JANUS_PLUGIN_H
#define _JANUS_PLUGIN_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <inttypes.h>


/*! \brief Version of the API, to match the one plugins were compiled against
 * 
 * \note This was added in version 0.0.7 of the gateway, to address changes
 * to the API that might break existing plugin or the core itself. All
 * plugins MUST implement the get_api_compatibility() method to make
 * this work. Do NOT try to launch a pre 0.0.7 plugin on a >= 0.0.7
 * gateway or it will crash.
 * 
 * */
#define JANUS_PLUGIN_API_VERSION	2


/*! \brief Callbacks to contact the gateway */
typedef struct janus_callbacks janus_callbacks;
/*! \brief The plugin session and callbacks interface */
typedef struct janus_plugin janus_plugin;
/*! \brief Plugin-Gateway session mapping */
typedef struct janus_plugin_session janus_plugin_session;
/*! \brief Result of individual requests passed to plugins */
typedef struct janus_plugin_result janus_plugin_result;

/*! \brief Plugin-Gateway session mapping */
struct janus_plugin_session {
	/*! \brief Opaque pointer to the gateway session */
	void *gateway_handle;
	/*! \brief Opaque pointer to the plugin session */
	void *plugin_handle;
	/*! \brief Whether this mapping has been stopped definitely or not: if so,
	 * the plugin shouldn't make use of it anymore */
	int stopped:1;
};

/*! \brief The plugin session and callbacks interface */
struct janus_plugin {
	/*! \brief Plugin initialization/constructor
	 * @param[in] callback The callback instance the plugin can use to contact the gateway
	 * @param[in] config_path Path of the folder where the configuration for this plugin can be found
	 * @returns 0 in case of success, a negative integer in case of error */
	int (* const init)(janus_callbacks *callback, const char *config_path);
	/*! \brief Plugin deinitialization/destructor */
	void (* const destroy)(void);

	/*! \brief Informative method to request the API version this plugin was compiled against
	 *  \note This was added in version 0.0.7 of the gateway, to address changes
	 * to the API that might break existing plugin or the core itself. All
	 * plugins MUST implement this method and return JANUS_PLUGIN_API_VERSION
	 * to make this work, or they will be rejected by the core. Do NOT try
	 * to launch a <= 0.0.7 plugin on a >= 0.0.7 gateway or it will crash. */
	int (* const get_api_compatibility)(void);
	/*! \brief Informative method to request the numeric version of the plugin */
	int (* const get_version)(void);
	/*! \brief Informative method to request the string version of the plugin */
	const char *(* const get_version_string)(void);
	/*! \brief Informative method to request a description of the plugin */
	const char *(* const get_description)(void);
	/*! \brief Informative method to request the name of the plugin */
	const char *(* const get_name)(void);
	/*! \brief Informative method to request the author of the plugin */
	const char *(* const get_author)(void);
	/*! \brief Informative method to request the package name of the plugin (what will be used in web applications to refer to it) */
	const char *(* const get_package)(void);

	/*! \brief Method to create a new session/handle for a peer
	 * @param[in] handle The plugin/gateway session that will be used for this peer
	 * @param[out] error An integer that may contain information about any error */
	void (* const create_session)(janus_plugin_session *handle, int *error);
	/*! \brief Method to handle an incoming message/request from a peer
	 * @param[in] handle The plugin/gateway session used for this peer
	 * @param[in] transaction The transaction identifier for this message/request
	 * @param[in] message The stringified version of the message/request JSON
	 * @returns A janus_plugin_result instance that may contain a response (for immediate/synchronous replies), an ack
	 * (for asynchronously managed requests) or an error */
	struct janus_plugin_result * (* const handle_message)(janus_plugin_session *handle, char *transaction, char *message, char *sdp_type, char *sdp);
	/*! \brief Callback to be notified when the associated PeerConnection is up and ready to be used
	 * @param[in] handle The plugin/gateway session used for this peer */
	void (* const setup_media)(janus_plugin_session *handle);
	/*! \brief Method to handle an incoming RTP packet from a peer
	 * @param[in] handle The plugin/gateway session used for this peer
	 * @param[in] video Whether this is an audio or a video frame
	 * @param[in] buf The packet data (buffer)
	 * @param[in] len The buffer lenght */
	void (* const incoming_rtp)(janus_plugin_session *handle, int video, char *buf, int len);
	/*! \brief Method to handle an incoming RTCP packet from a peer
	 * @param[in] handle The plugin/gateway session used for this peer
	 * @param[in] video Whether this is related to an audio or a video stream
	 * @param[in] buf The message data (buffer)
	 * @param[in] len The buffer lenght */
	void (* const incoming_rtcp)(janus_plugin_session *handle, int video, char *buf, int len);
	/*! \brief Method to handle incoming SCTP/DataChannel data from a peer (text only, for the moment)
	 * \note We currently only support text data, binary data will follow... please also notice that
	 * DataChannels send unterminated strings, so you'll have to terminate them with a \0 yourself to
	 * use them. 
	 * @param[in] handle The plugin/gateway session used for this peer
	 * @param[in] buf The message data (buffer)
	 * @param[in] len The buffer lenght */
	void (* const incoming_data)(janus_plugin_session *handle, char *buf, int len);
	/*! \brief Callback to be notified about DTLS alerts from a peer (i.e., the PeerConnection is not valid any more)
	 * @param[in] handle The plugin/gateway session used for this peer */
	void (* const hangup_media)(janus_plugin_session *handle);
	/*! \brief Method to destroy a session/handle for a peer
	 * @param[in] handle The plugin/gateway session used for this peer
	 * @param[out] error An integer that may contain information about any error */
	void (* const destroy_session)(janus_plugin_session *handle, int *error);
	/*! \brief Method to get plugin-specific info of a session/handle
	 *  \note This was added in version 0.0.7 of the gateway. Janus assumes
	 * the string is always allocated, so don't return constants here
	 * @param[in] handle The plugin/gateway session used for this peer
	 * @returns A JSON-formatted string with the requested info */
	char *(* const query_session)(janus_plugin_session *handle);

};

/*! \brief Callbacks to contact the gateway */
struct janus_callbacks {
	/*! \brief Callback to push events/messages to a peer
	 * @param[in] handle The plugin/gateway session used for this peer
	 * @param[in] plugin The plugin instance that is sending the message/event
	 * @param[in] transaction The transaction identifier this message refers to
	 * @param[in] message The stringified version of the JSON message
	 * @param[in] sdp_type The type of the SDP attached to the message/event, if any (offer/answer)
	 * @param[in] sdp The SDP attached to the message/event, if any (in case the plugin is requesting or responding to a media setup) */
	int (* const push_event)(janus_plugin_session *handle, janus_plugin *plugin, const char *transaction, const char *message, const char *sdp_type, const char *sdp);

	/*! \brief Callback to relay RTP packets to a peer
	 * @param[in] handle The plugin/gateway session used for this peer
	 * @param[in] video Whether this is an audio or a video frame
	 * @param[in] buf The packet data (buffer)
	 * @param[in] len The buffer lenght */
	void (* const relay_rtp)(janus_plugin_session *handle, int video, char *buf, int len);
	/*! \brief Callback to relay RTCP messages to a peer
	 * @param[in] handle The plugin/gateway session that will be used for this peer
	 * @param[in] video Whether this is related to an audio or a video stream
	 * @param[in] buf The message data (buffer)
	 * @param[in] len The buffer lenght */
	void (* const relay_rtcp)(janus_plugin_session *handle, int video, char *buf, int len);
	/*! \brief Callback to relay SCTP/DataChannel messages to a peer
	 * @param[in] handle The plugin/gateway session that will be used for this peer
	 * @param[in] buf The message data (buffer)
	 * @param[in] len The buffer lenght */
	void (* const relay_data)(janus_plugin_session *handle, char *buf, int len);

	/*! \brief Callback to ask the core to close a WebRTC PeerConnection
	 * \note A call to this method will result in the core invoking the hangup_media
	 * callback on this plugin when done
	 * @param[in] handle The plugin/gateway session that the PeerConnection is related to */
	void (* const close_pc)(janus_plugin_session *handle);
	/*! \brief Callback to ask the core to get rid of a plugin/gateway session
	 * \note A call to this method will result in the core invoking the destroy_session
	 * callback on this plugin when done
	 * @param[in] handle The plugin/gateway session to get rid of */
	void (* const end_session)(janus_plugin_session *handle);

};

/*! \brief The hook that plugins need to implement to be created from the gateway */
typedef janus_plugin* create_p(void);


/** @name Janus plugin results
 * @brief When a client sends a message to a plugin (e.g., a request or a
 * command) this is notified to the plugin through a handle_message()
 * callback. The plugin can then either handle the request immediately
 * and provide a response (synchronous approach) or decide to queue it
 * and process it later (asynchronous approach). In both cases the plugin
 * must return a janus_plugin_result instance to the core, that will allow
 * the client to: 1. know whether a response is immediately available or
 * it will be later on through notifications, and 2. what the actual content
 * of the result might be. Of course, notifications related to the
 * transaction may occur later on even for synchronous requests, if the
 * plugin was implemented with use cases that envisage this approach.
 * @note An error may be returned as well, but this would cause a core-level
 * error to be returned to the client. If you want to provide indications
 * about a failed operation for application-level reason, the correct
 * approach is to return a success with a plugin-specific payload describing
 * the error.
 */
///@{
/*! \brief Result types */
typedef enum janus_plugin_result_type {
	/*! \brief A severe error happened (not an application level error) */
	JANUS_PLUGIN_ERROR = -1,
	/*! \brief The request was correctly handled and a response is provided (synchronous) */
	JANUS_PLUGIN_OK,
	/*! \brief The request was correctly handled and notifications will follow with more info (asynchronous) */
	JANUS_PLUGIN_OK_WAIT,
} janus_plugin_result_type;

/*! \brief Janus plugin result */
struct janus_plugin_result {
	/*! \brief Result type */
	janus_plugin_result_type type;
	/*! \brief Result content
	 * @note This MUST be a valid JSON payload in case of JANUS_PLUGIN_OK (even when
	 * returning application level errors), and MUST be a generic string
	 * in case of JANUS_PLUGIN_ERROR. If returning a JANUS_PLUGIN_OK_WAIT,
	 * the content is optional, but if provided (e.g., if the plugin wants
	 * to provide verbose info about the reasons for choosing an async approach)
	 * the content MUST be a generic string, as it will end up in a string
	 * element in the JSON ack response. */
	char *content;
};

/*! \brief Helper to quickly create a janus_plugin_result instance
 * @param[in] type The type of result
 * @param[in] content The content of the result
 * @note The content is always strdup-ed, if available, so remember to free the original
 * string yourself, if you allocated it
 * @returns A valid janus_plugin_result instance, if successful, or NULL otherwise */
janus_plugin_result *janus_plugin_result_new(janus_plugin_result_type type, const char *content);

/*! \brief Helper to quickly destroy a janus_plugin_result instance
 * @param[in] result The janus_plugin_result instance to destroy
 * @returns A valid janus_plugin_result instance, if successful, or NULL otherwise */
void janus_plugin_result_destroy(janus_plugin_result *result);
///@}


#endif
