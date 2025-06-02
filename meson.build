project('gtkbar', 'c', version: '0.0.1')

gtk = dependency('gtk4')
gtk4layershell = dependency('gtk4-layer-shell-0')
cjson = dependency('libcjson')

deps = [gtk, gtk4layershell, cjson]

src = [
  'main.c',
  'app.h',
  'battery.c',
  'date.c',
  'wifi.c',
  'workspaces.c',
  'util.c',
  'volume.c',
  'volumepopup.h',
  'volumepopup.c',
  'calendarpopup.c',
  'calendarpopup.h',
]

install_data('style.css', install_dir: join_paths(get_option('datadir'), 'gtkbar'))

executable('gtkbar', sources: src, dependencies: deps, install: true)
