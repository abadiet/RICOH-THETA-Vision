~~~bash
mkdir thetax
cd thetax/
git clone https://github.com/ricohapi/libuvc-theta-sample.git
git clone https://github.com/ricohapi/libuvc-theta.git
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libusb-1.0-0-dev libjpeg-dev cmake
cd libuvc-theta
mkdir build
cd build/
cmake ..
sudo make install
cd ../../libuvc-theta-sample/gst/
~~~

Patch thetauvc.c :

~~~git-patch
diff --git a/gst/thetauvc.c b/gst/thetauvc.c
index a9535d7..3a3d73a 100644
--- a/gst/thetauvc.c
+++ b/gst/thetauvc.c
@@ -41,6 +41,7 @@
 #define USBVID_RICOH 0x05ca
 #define USBPID_THETAV_UVC 0x2712
 #define USBPID_THETAZ1_UVC 0x2715
+#define USBPID_THETAX_UVC 0x2717
 
 
 struct thetauvc_mode {
@@ -102,7 +103,8 @@ thetauvc_find_devices(uvc_context_t *ctx, uvc_device_t ***devs)
 			continue;
 
 		if (desc->idProduct == USBPID_THETAV_UVC
-			|| desc->idProduct == USBPID_THETAZ1_UVC) {
+			|| desc->idProduct == USBPID_THETAZ1_UVC
+			|| desc->idProduct == USBPID_THETAX_UVC) {
 			void *tmp_ptr;
 
 			devcnt++;
~~~

~~~bash
make
export LD_LIBRARY_PATH=~/thetax/libuvc-theta/build/
./gst_viewer
~~~