/* alsa_rnnoise - rnnoise-based alsa noise removal
 * Copyright (C) 2021  Arsen Arsenović <arsen@aarsen.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>
#include <rnnoise.h>

#ifdef DNO_RNNOISE_GET_FRAME_SIZE
/* backwards compat with older rnnoise revisions */
static inline int rnnoise_get_frame_size() {
	return 480;
}
#endif

typedef struct {
	snd_pcm_extplug_t ext;
	DenoiseState *rnnoise;

	float *buf;
	size_t filled;
} alsa_rnnoise_info;

static void *area_start_address(
	const snd_pcm_channel_area_t *area,
	snd_pcm_uframes_t offset
) {
	size_t bit_offset = area->first + area->step * offset;
	assert(bit_offset % 8 == 0); /* always true? */
	return (char *) area->addr + bit_offset / 8;
}

static snd_pcm_sframes_t
arnn_transfer(
	snd_pcm_extplug_t *ext,
	const snd_pcm_channel_area_t *dst_area,
	snd_pcm_uframes_t dst_offset,
	const snd_pcm_channel_area_t *src_area,
	snd_pcm_uframes_t src_offset,
	snd_pcm_uframes_t size
) {
	alsa_rnnoise_info *pdata = ext->private_data;
	int16_t *src = area_start_address(src_area, src_offset);
	int16_t *dst = area_start_address(dst_area, dst_offset);
	size_t count = size;

	/* this is pretty much the algorithm in pcm_speex.c in alsa-plugins
	 * I've reorganized it to fit code style and added code to handle the
	 * S16 -> fake float conversion.
	 */
	while (count > 0) {
		size_t chunk = rnnoise_get_frame_size() - pdata->filled;
		if (chunk > count) {
			chunk = count;
		}

		for (size_t i = 0; i < chunk; i++) {
			dst[i] = pdata->buf[pdata->filled + i];
			pdata->buf[pdata->filled + i] = src[i];
		}
		dst += chunk;

		pdata->filled += chunk;

		src += chunk;
		count -= chunk;

		if (pdata->filled != rnnoise_get_frame_size()) {
			continue;
		}
		rnnoise_process_frame(pdata->rnnoise, pdata->buf, pdata->buf);
		pdata->filled = 0;
	}

	return size;
}

static int arnn_init(snd_pcm_extplug_t *ext) {
	alsa_rnnoise_info *pdata = ext->private_data;

	pdata->filled = 0;

	pdata->buf = realloc(pdata->buf,
			rnnoise_get_frame_size() * sizeof(float));
	if (!pdata->buf) {
		return -ENOMEM;
	}
	memset(pdata->buf, 0, rnnoise_get_frame_size() * sizeof(float));

	if (pdata->rnnoise) {
		rnnoise_destroy(pdata->rnnoise);
	}

	pdata->rnnoise = rnnoise_create(NULL);
	if (!pdata->rnnoise) {
		return -ENOMEM;
	}
	return 0;
}

static int arnn_close(snd_pcm_extplug_t *ext) {
	alsa_rnnoise_info *pdata = ext->private_data;
	free(pdata->buf);
	/* rnnoise_destroy does not null check */
	if (pdata->rnnoise) {
		rnnoise_destroy(pdata->rnnoise);
	}
	return 0;
}

static const snd_pcm_extplug_callback_t rnnoise_callback = {
	.transfer = arnn_transfer,
	.init = arnn_init,
	.close = arnn_close,
};

SND_PCM_PLUGIN_DEFINE_FUNC(rnnoise) {
	snd_config_iterator_t i, next;
	alsa_rnnoise_info *arnn;
	snd_config_t *slave = NULL;
	int err;

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0) {
			continue;
		}
		if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0) {
			continue;
		}
		if (strcmp(id, "slave") == 0) {
			slave = n;
			continue;
		}
		SNDERR("unknown field %s", id);
		return -EINVAL;
	}

	if (!slave) {
		SNDERR("no slave configuration for rnnoise pcm");
		return -EINVAL;
	}

	arnn = calloc(1, sizeof(*arnn));
	if (!arnn) {
		return -ENOMEM;
	}

	arnn->ext.version = SND_PCM_EXTPLUG_VERSION;
	arnn->ext.name = "rnnoise-based denoiser for alsa";
	arnn->ext.callback = &rnnoise_callback;
	arnn->ext.private_data = arnn;

	err = snd_pcm_extplug_create(&arnn->ext, name, root, slave,
				     stream, mode);
	if (err < 0) {
		free(arnn);
		return err;
	}

	snd_pcm_extplug_set_param(&arnn->ext, SND_PCM_EXTPLUG_HW_CHANNELS, 1);
	snd_pcm_extplug_set_slave_param(&arnn->ext,
					SND_PCM_EXTPLUG_HW_CHANNELS, 1);

	/* RNNoise, for some reason, does not work for floats ranged -1 to 1
	 * but does work with floats in the int16_t range, so we request that
	 * and convert to floats in the transfer function
	 */
	snd_pcm_extplug_set_param(&arnn->ext, SND_PCM_EXTPLUG_HW_FORMAT,
				  SND_PCM_FORMAT_S16);
	snd_pcm_extplug_set_slave_param(&arnn->ext, SND_PCM_EXTPLUG_HW_FORMAT,
					SND_PCM_FORMAT_S16);

	*pcmp = arnn->ext.pcm;
	return 0;
}

SND_PCM_PLUGIN_SYMBOL(rnnoise);
