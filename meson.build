project('alsa_rnnoise', 'c')

rnnoise = dependency('rnnoise')
alsa = dependency('alsa')

pdir = get_option('plugin_dir')
if pdir == ''
	pdir = get_option('prefix') / get_option('libdir') / 'alsa-lib'
endif

cc = meson.get_compiler('c')
has_frame_func = cc.has_function('rnnoise_get_frame_size',
				prefix: '#include <rnnoise.h>',
				dependencies: rnnoise)

if not has_frame_func
	add_project_arguments('-DNO_RNNOISE_GET_FRAME_SIZE', language: 'c')
endif

summary({
	'plugin directory': pdir,
	'rnnoise_get_frame_size() detected?': has_frame_func
}, section: 'build configuration')

shared_library('asound_module_pcm_rnnoise',
	'src/main.c',
	dependencies: [rnnoise, alsa],
	install_dir: pdir,
	c_args: ['-DPIC'],
	install: true)
