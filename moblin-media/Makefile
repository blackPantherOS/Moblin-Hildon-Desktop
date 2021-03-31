BINDIR = ${DESTDIR}/usr/bin/
MEDIAAPPDIR = ${DESTDIR}/usr/share/moblin-media
MEDIASTREAMDIR = ${DESTDIR}/usr/share/moblin-media/media
THEME_DIR = ${DESTDIR}/usr/share/themes/mobilebasic
IMAGE_DIR = ${THEME_DIR}/images
MEDIADIR = ${MEDIAAPPDIR}/sample-media
IMPORTDIR = ${MEDIAAPPDIR}/import

all:
	# Do nothing

basicinstall:
	@mkdir -p ${BINDIR}
	@mkdir -p ${MEDIAAPPDIR}
	@mkdir -p ${MEDIASTREAMDIR}/video
	@mkdir -p ${MEDIASTREAMDIR}/audio
	@mkdir -p ${MEDIASTREAMDIR}/photo
	@mkdir -p ${MEDIASTREAMDIR}/.playlist
	@mkdir -p ${DESTDIR}/usr/share/media-services

	#for moblin-media
	@mkdir -p ${MEDIADIR}
	@mkdir -p ${MEDIADIR}/video
	@mkdir -p ${MEDIADIR}/audio
	@mkdir -p ${MEDIADIR}/photo
	@mkdir -p ${MEDIADIR}/.playlist

	@mkdir -p ${THEME_DIR}
	@mkdir -p ${MEDIAAPPDIR}/config
	@mkdir -p ${MEDIAAPPDIR}/images
	@head -n 1 debian/changelog > ${MEDIAAPPDIR}/version || head -n 1 ChangeLog > ${MEDIAAPPDIR}/version
	@install -m 644 -D images/thumbnail_background.png ${IMAGE_DIR}/thumbnail_background.png
	@install -m 644 -D images/AlbumArtMissing.png ${MEDIAAPPDIR}/images/AlbumArtMissing.png
	@install -m 644 -D images/AlbumArtMissingOnline.png ${MEDIAAPPDIR}/images/AlbumArtMissingOnline.png
	@install -m 644 -D images/about.png ${MEDIAAPPDIR}/images/about.png
	@install -m 644 -D images/invalid.png ${MEDIAAPPDIR}/images/invalid.png
	@install -m 644 -D images/mime-video.png ${IMAGE_DIR}/mime-video.png
	@install -m 644 -D images/mime-audio.png ${IMAGE_DIR}/mime-audio.png
	@install -m 644 -D images/mime-photo.png ${IMAGE_DIR}/mime-photo.png
	@install -m 644 -D dbus/org.moblin.media.service ${DESTDIR}/usr/share/dbus-1/services/org.moblin.media.service
	@install -m 644 -D dbus/org.moblin.media.video.service ${DESTDIR}/usr/share/dbus-1/services/org.moblin.media.video.service
	@install -m 644 -D dbus/org.moblin.media.music.service ${DESTDIR}/usr/share/dbus-1/services/org.moblin.media.music.service
	@install -m 644 -D dbus/org.moblin.media.photo.service ${DESTDIR}/usr/share/dbus-1/services/org.moblin.media.photo.service

	@install -m 755 -D src/top_menu.py ${MEDIAAPPDIR}/top_menu.py
	@install -m 755 -D src/playlist.py ${MEDIAAPPDIR}/playlist.py
	@install -m 755 -D src/media_service.py ${MEDIAAPPDIR}/media_service.py
	@install -m 755 -D src/media_info.py ${MEDIAAPPDIR}/media_info.py
	@install -m 755 -D src/media_info.py ${MEDIAAPPDIR}/media_info.py
	@install -m 755 -D src/settings_dialog.py ${MEDIAAPPDIR}/settings_dialog.py
	@install -m 755 -D src/utils.py ${MEDIAAPPDIR}/utils.py
	@install -m 755 -D src/image_finger.py ${MEDIAAPPDIR}/image_finger.py
	@install -m 755 -D src/photo_view.py ${MEDIAAPPDIR}/photo_view.py
	@install -m 755 -D src/toolbar_view.py ${MEDIAAPPDIR}/toolbar_view.py
	@install -m 755 -D src/music_view.py ${MEDIAAPPDIR}/music_view.py
	@install -m 755 -D src/video_view.py ${MEDIAAPPDIR}/video_view.py
	@install -m 755 -D src/constant.py ${MEDIAAPPDIR}/constant.py
	@install -m 755 -D src/app.py ${MEDIAAPPDIR}/app.py
	@install -m 755 -D src/thumbnail_view.py ${MEDIAAPPDIR}/thumbnail_view.py
	@install -m 755 -D src/photo_playback.py ${MEDIAAPPDIR}/photo_playback.py
	@install -m 755 -D src/media_plugin.py ${MEDIAAPPDIR}/media_plugin.py
	@install -m 644 -D src/thumbnail_creator.py ${MEDIAAPPDIR}/thumbnail_creator.py
	@install -m 644 -D src/thumb_base.py ${MEDIAAPPDIR}/thumb_base.py
	@install -m 644 -D src/hint_window.py ${MEDIAAPPDIR}/hint_window.py

	@install -m 644 -D glade/moblin_media.glade ${MEDIAAPPDIR}/moblin_media.glade
	@install -m 644 -D glade/settings_dialog.glade ${MEDIAAPPDIR}/settings_dialog.glade
	@install -m 644 -D glade/dialogs.glade ${MEDIAAPPDIR}/dialogs.glade

	@install -m 755 -D moblin-media ${BINDIR}/moblin-media
	@install -m 644 -D desktop/moblin-media-video.desktop ${DESTDIR}/usr/share/applications/moblin-media-video.desktop
	@install -m 644 -D desktop/moblin-media-music.desktop ${DESTDIR}/usr/share/applications/moblin-media-music.desktop
	@install -m 644 -D desktop/moblin-media-photo.desktop ${DESTDIR}/usr/share/applications/moblin-media-photo.desktop

	@mkdir -p ${DESTDIR}/usr/share/icons/hicolor/32x32/apps/
	@install -m 644 -D icons/32x32/apps/* ${DESTDIR}/usr/share/icons/hicolor/32x32/apps/

	@mkdir -p ${DESTDIR}/usr/share/icons/hicolor/48x48/apps/
	@install -m 644 -D icons/48x48/apps/* ${DESTDIR}/usr/share/icons/hicolor/48x48/apps/

	@mkdir -p ${DESTDIR}/usr/share/icons/hicolor/64x64/apps/
	@install -m 644 -D icons/64x64/apps/* ${DESTDIR}/usr/share/icons/hicolor/64x64/apps/

	@mkdir -p ${DESTDIR}/usr/share/icons/hicolor/72x72/apps/
	@install -m 644 -D icons/72x72/apps/* ${DESTDIR}/usr/share/icons/hicolor/72x72/apps/

	@mkdir -p ${DESTDIR}/usr/share/icons/hicolor/96x96/apps/
	@install -m 644 -D icons/96x96/apps/* ${DESTDIR}/usr/share/icons/hicolor/96x96/apps/

	@mkdir -p ${DESTDIR}/usr/share/gconf/schemas
	@install -m 644 -D moblin-media.schemas ${DESTDIR}/usr/share/gconf/schemas/moblin-media.schemas

	#for moblin-media-sample-content
	@install -m 644 -D content/photo/* ${MEDIADIR}/photo/
	@install -m 644 -D content/video/* ${MEDIADIR}/video/
	@install -m 644 -D content/audio/* ${MEDIADIR}/audio/

	#for moblin-media-import
	@mkdir -p ${IMPORTDIR}
	@install -m 755 -D import/media-import ${BINDIR}/media-import
	@install -m 644 -D import/media-import.desktop ${DESTDIR}/usr/share/gnome/autostart/media-import.desktop
	@install -m 644 -D import/media-import.glade ${MEDIAAPPDIR}/media-import.glade

install: basicinstall runbasictests
		@${MAKE} -C po install

runbasictests:
	@echo "Running basic unit tests..."
	src/unittest/testMoblinMedia.py
	@echo "Basic unit tests completed"


clean:
	rm -f *~
