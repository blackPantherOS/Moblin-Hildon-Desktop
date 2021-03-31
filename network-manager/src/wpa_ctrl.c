/*
 * wpa_supplicant/hostapd control interface library
 * Copyright (c) 2004-2005, Jouni Malinen <jkmaline@cc.hut.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

/* WHACK #include "includes.h" */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#define CONFIG_CTRL_IFACE
#ifdef CONFIG_CTRL_IFACE

#ifndef CONFIG_CTRL_IFACE_UDP
#include <sys/un.h>
#endif /* CONFIG_CTRL_IFACE_UDP */

#include "wpa_ctrl.h"
/* WHACK #include "common.h" */

/**
 * struct wpa_ctrl - Internal structure for control interface library
 *
 * This structure is used by the wpa_supplicant/hostapd control interface
 * library to store internal data. Programs using the library should not touch
 * this data directly. They can only use the pointer to the data structure as
 * an identifier for the control interface connection and use this as one of
 * the arguments for most of the control interface library functions.
 */
struct wpa_ctrl {
	int s;
#ifdef CONFIG_CTRL_IFACE_UDP
	struct sockaddr_in local;
	struct sockaddr_in dest;
#else /* CONFIG_CTRL_IFACE_UDP */
	struct sockaddr_un local;
	struct sockaddr_un dest;
#endif /* CONFIG_CTRL_IFACE_UDP */
  int timeout_sec;
};

void wpa_ctrl_set_custom_timeout(struct wpa_ctrl *ctrl, int timeout_sec)
{
  ctrl->timeout_sec = timeout_sec;
}


void wpa_ctrl_unset_custom_timeout(struct wpa_ctrl *ctrl)
{
  ctrl->timeout_sec = 2;
}

struct wpa_ctrl * wpa_ctrl_open(const char *ctrl_path,
                                const char *local_path_dir)
{
	struct wpa_ctrl *ctrl;
#ifndef CONFIG_CTRL_IFACE_UDP
	static int counter = 0;
#endif /* CONFIG_CTRL_IFACE_UDP */

	ctrl = malloc(sizeof(*ctrl));
	if (ctrl == NULL)
		return NULL;
	memset(ctrl, 0, sizeof(*ctrl));

	ctrl->timeout_sec = 2;

#ifdef CONFIG_CTRL_IFACE_UDP
	ctrl->s = socket(PF_INET, SOCK_DGRAM, 0);
	if (ctrl->s < 0) {
		perror("socket");
		free(ctrl);
		return NULL;
	}

	ctrl->local.sin_family = AF_INET;
	ctrl->local.sin_addr.s_addr = htonl((127 << 24) | 1);
	if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
		 sizeof(ctrl->local)) < 0) {
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}

	ctrl->dest.sin_family = AF_INET;
	ctrl->dest.sin_addr.s_addr = htonl((127 << 24) | 1);
	ctrl->dest.sin_port = htons(WPA_CTRL_IFACE_PORT);
	if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
		    sizeof(ctrl->dest)) < 0) {
		perror("connect");
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}
#else /* CONFIG_CTRL_IFACE_UDP */
	ctrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (ctrl->s < 0) {
		free(ctrl);
		return NULL;
	}
	fcntl(ctrl->s, F_SETFD, fcntl(ctrl->s, F_GETFD) | O_NONBLOCK);

	ctrl->local.sun_family = AF_UNIX;
	snprintf(ctrl->local.sun_path, sizeof(ctrl->local.sun_path),
		 "%s/wpa_ctrl_%d-%d", local_path_dir, getpid(), counter++);
	if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
		    sizeof(ctrl->local)) < 0) {
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}

	ctrl->dest.sun_family = AF_UNIX;
	snprintf(ctrl->dest.sun_path, sizeof(ctrl->dest.sun_path), "%s",
		 ctrl_path);
	if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
		    sizeof(ctrl->dest)) < 0) {
		close(ctrl->s);
		unlink(ctrl->local.sun_path);
		free(ctrl);
		return NULL;
	}
#endif /* CONFIG_CTRL_IFACE_UDP */

	return ctrl;
}


void wpa_ctrl_close(struct wpa_ctrl *ctrl)
{
#ifndef CONFIG_CTRL_IFACE_UDP
	unlink(ctrl->local.sun_path);
#endif /* CONFIG_CTRL_IFACE_UDP */
	close(ctrl->s);
	free(ctrl);
}


static int wpa_ctrl_request_timed(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
				  char *reply, size_t *reply_len, int timeout_sec,
				  void (*msg_cb)(char *msg, size_t len));

int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
		     char *reply, size_t *reply_len,
		     void (*msg_cb)(char *msg, size_t len))
{
  return wpa_ctrl_request_timed(ctrl, cmd, cmd_len, reply, reply_len, ctrl->timeout_sec, msg_cb);
}

static int wpa_ctrl_request_timed(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
				  char *reply, size_t *reply_len, int timeout_sec,
				  void (*msg_cb)(char *msg, size_t len))
{
	struct timeval tv;
	int res;
	fd_set rfds;

	if (send(ctrl->s, cmd, cmd_len, 0) < 0)
		return -1;

	for (;;) {
		tv.tv_sec = timeout_sec;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(ctrl->s, &rfds);
		res = select(ctrl->s + 1, &rfds, NULL, NULL, &tv);
		if (res >0 && FD_ISSET(ctrl->s, &rfds)) {
			res = recv(ctrl->s, reply, *reply_len, 0);
			if (res < 0)
				return res;
			if (res > 0 && reply[0] == '<') {
				/* This is an unsolicited message from
				 * wpa_supplicant, not the reply to the
				 * request. Use msg_cb to report this to the
				 * caller. */
				if (msg_cb) {
					/* Make sure the message is nul
					 * terminated. */
					if ((size_t) res == *reply_len)
						res = (*reply_len) - 1;
					reply[res] = '\0';
					msg_cb(reply, res);
				}
				continue;
			}
			*reply_len = res;
			break;
		} else {
			sprintf(reply, "TIMEOUT[CLI]\0");
			*reply_len = strlen("TIMEOUT[CLI]\0");
			return -2;
		}
	}
	return 0;
}


static int wpa_ctrl_attach_helper(struct wpa_ctrl *ctrl, int attach)
{
	char buf[10];
	int ret;
	size_t len = 10;

	ret = wpa_ctrl_request(ctrl, attach ? "ATTACH" : "DETACH", 6,
			       buf, &len, NULL);
	if (ret < 0)
		return ret;
	if (len == 3 && memcmp(buf, "OK\n", 3) == 0)
		return 0;
	return -1;
}


int wpa_ctrl_attach(struct wpa_ctrl *ctrl)
{
	return wpa_ctrl_attach_helper(ctrl, 1);
}


int wpa_ctrl_detach(struct wpa_ctrl *ctrl)
{
	return wpa_ctrl_attach_helper(ctrl, 0);
}


int wpa_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len)
{
	int res;

	res = recv(ctrl->s, reply, *reply_len, 0);
	if (res < 0)
		return res;
	*reply_len = res;
	return 0;
}


int wpa_ctrl_pending(struct wpa_ctrl *ctrl)
{
	struct timeval tv;
	int res;
	fd_set rfds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	FD_SET(ctrl->s, &rfds);
	res = select(ctrl->s + 1, &rfds, NULL, NULL, &tv);
	return res > 0 && FD_ISSET(ctrl->s, &rfds);
}


int wpa_ctrl_get_fd(struct wpa_ctrl *ctrl)
{
	return ctrl->s;
}

#endif /* CONFIG_CTRL_IFACE */
