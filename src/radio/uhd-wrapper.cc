
#include <radio/uhd-wrapper.h>

#include <complex>

#include <uhd/stream.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>

struct uhd_wrapper {
    uhd::usrp::multi_usrp::sptr musrp;
    uhd::rx_streamer::sptr rxs;
    int flags;
#define UHD_WRAPPER_FLAG_STREAMING	1
};

uhd_wrapper *
uhd_wrapper_open(const char *args, const char *spec, const char *ant) {
    uhd_wrapper *u = new uhd_wrapper();
    uhd::device_addr_t addr(args);
    try {
	u->musrp = uhd::usrp::multi_usrp::make(addr);
    } catch (...) {
	u->musrp = NULL;
    }
    if (u->musrp == NULL) {
	delete u;
	return NULL;
    }
    if (spec != NULL && spec[0] != '\0') {
	uhd::usrp::subdev_spec_t sp(spec);
	u->musrp->set_rx_subdev_spec(sp);
    }
    if (ant != NULL && ant[0] != '\0')
	u->musrp->set_rx_antenna(ant);
    u->rxs = u->musrp->get_rx_stream(uhd::stream_args_t("fc64", "sc16"));
    u->flags = 0;

    return u;
}

void
uhd_wrapper_close(uhd_wrapper *u) {
    if (u->flags & UHD_WRAPPER_FLAG_STREAMING) {
	u->rxs->issue_stream_cmd(uhd::stream_cmd_t(
		uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS));
    }

    delete u;
}

int
uhd_wrapper_set_frequency(uhd_wrapper *u, double freq) {
    if (u->flags & UHD_WRAPPER_FLAG_STREAMING)
	u->rxs->issue_stream_cmd(uhd::stream_cmd_t(
		uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS));

    u->musrp->set_rx_freq(uhd::tune_request_t(freq));

    if (u->flags & UHD_WRAPPER_FLAG_STREAMING)
	u->rxs->issue_stream_cmd(uhd::stream_cmd_t(
		uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS));
    return 0;
}

double
uhd_wrapper_get_frequency(uhd_wrapper *u) {
    return u->musrp->get_rx_freq();
}

int
uhd_wrapper_set_sample_rate(uhd_wrapper *u, double sample_rate) {
    u->musrp->set_rx_rate(sample_rate);

    return 0;
}

double
uhd_wrapper_get_sample_rate(uhd_wrapper *u) {
    return u->musrp->get_rx_rate();
}

#include <iostream>

size_t
uhd_wrapper_read(uhd_wrapper *u, void *buf, size_t len) {
    uhd::rx_streamer::buffs_type buffs(buf);
    uhd::rx_metadata_t metadata;

    if (!(u->flags & UHD_WRAPPER_FLAG_STREAMING)) {
	uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
	cmd.stream_now = true;
	u->rxs->issue_stream_cmd(cmd);
	u->flags |= UHD_WRAPPER_FLAG_STREAMING;
    }

    return u->rxs->recv(buffs, len, metadata, 100);
}
