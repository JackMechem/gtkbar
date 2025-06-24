project('gtkbar', 'c', version: '0.0.1')

gtk = dependency('gtk4')
gtk4layershell = dependency('gtk4-layer-shell-0')
cjson = dependency('libcjson')
libnm = dependency('libnm')
libpulse = dependency('libpulse')

deps = [gtk, gtk4layershell, cjson, libnm, libpulse]

src = [
  'src/main.c',
  'src/app.h',
  'src/util.c',
  'src/widgets/battery.c',
  'src/widgets/date.c',
  'src/widgets/wifi.c',
  'src/widgets/wifi.h',
  'src/widgets/workspaces.c',
  'src/widgets/volume.c',
  'src/widgets/volume.h',
  'src/popups/volumepopup.h',
  'src/popups/volumepopup.c',
  'src/popups/calendarpopup.c',
  'src/popups/calendarpopup.h',
  'src/popups/ctrlcenterpopup.c',
  'src/popups/ctrlcenterpopup.h',
  'src/popups/mixer.h',
  'src/popups/mixer.c',
  'src/popups/bt.h',
  'src/popups/bt.c',
  'src/popups/popupposition.h',
  'src/popups/popupposition.c',
  'src/bar/bar.c',
  'src/bar/bar.h',
  'src/media/media.c',
  'src/media/media.h',
  'src/networking/networking.h',
  'src/networking/networking.c',
]

inc_dirs = include_directories('src', 'src/widgets', 'src/popups', 'src/bar', 'src/media', 'src/networking')

install_data('style.css', install_dir: join_paths(get_option('datadir'), 'gtkbar'))

exe_target = executable(
  'gtkbar',
  sources: src,
  include_directories: inc_dirs,
  dependencies: deps,
  install: true,
)

run_target('rundev', command: [exe_target, '--css-path', '../style.css'])
