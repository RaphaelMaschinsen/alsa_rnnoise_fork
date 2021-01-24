project('alsa_rnnoise', 'c')

pdir = get_option('plugin_dir')
if pdir == ''
	pdir = get_option('prefix') / get_option('libdir') / 'alsa-lib'
endif

summary({
	'plugin directory': pdir
}, section: 'build configuration')

rnnoise = dependency('rnnoise')
alsa = dependency('alsa')

shared_library('asound_module_pcm_rnnoise',
	'src/main.c',
	dependencies: [rnnoise, alsa],
	install_dir: pdir,
	c_args: ['-DPIC'],
	install: true)
