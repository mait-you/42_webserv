#include "../../includes/MimeTypes.hpp"

Mime::MimeMap Mime::_types = Mime::_initTypes();

Mime::MimeMap Mime::_initTypes() {
	Mime::MimeMap t;
	t["html"]	 = "text/html";
	t["htm"]	 = "text/html";
	t["shtml"]	 = "text/html";
	t["css"]	 = "text/css";
	t["xml"]	 = "text/xml";
	t["gif"]	 = "image/gif";
	t["jpeg"]	 = "image/jpeg";
	t["jpg"]	 = "image/jpeg";
	t["js"]		 = "application/javascript";
	t["atom"]	 = "application/atom+xml";
	t["rss"]	 = "application/rss+xml";
	t["mml"]	 = "text/mathml";
	t["txt"]	 = "text/plain";
	t["jad"]	 = "text/vnd.sun.j2me.app-descriptor";
	t["wml"]	 = "text/vnd.wap.wml";
	t["htc"]	 = "text/x-component";
	t["avif"]	 = "image/avif";
	t["png"]	 = "image/png";
	t["svg"]	 = "image/svg+xml";
	t["svgz"]	 = "image/svg+xml";
	t["tif"]	 = "image/tiff";
	t["tiff"]	 = "image/tiff";
	t["wbmp"]	 = "image/vnd.wap.wbmp";
	t["webp"]	 = "image/webp";
	t["ico"]	 = "image/x-icon";
	t["jng"]	 = "image/x-jng";
	t["bmp"]	 = "image/x-ms-bmp";
	t["woff"]	 = "font/woff";
	t["woff2"]	 = "font/woff2";
	t["jar"]	 = "application/java-archive";
	t["war"]	 = "application/java-archive";
	t["ear"]	 = "application/java-archive";
	t["json"]	 = "application/json";
	t["hqx"]	 = "application/mac-binhex40";
	t["doc"]	 = "application/msword";
	t["pdf"]	 = "application/pdf";
	t["ps"]		 = "application/postscript";
	t["eps"]	 = "application/postscript";
	t["ai"]		 = "application/postscript";
	t["rtf"]	 = "application/rtf";
	t["m3u8"]	 = "application/vnd.apple.mpegurl";
	t["kml"]	 = "application/vnd.google-earth.kml+xml";
	t["kmz"]	 = "application/vnd.google-earth.kmz";
	t["xls"]	 = "application/vnd.ms-excel";
	t["eot"]	 = "application/vnd.ms-fontobject";
	t["ppt"]	 = "application/vnd.ms-powerpoint";
	t["odg"]	 = "application/vnd.oasis.opendocument.graphics";
	t["odp"]	 = "application/vnd.oasis.opendocument.presentation";
	t["ods"]	 = "application/vnd.oasis.opendocument.spreadsheet";
	t["odt"]	 = "application/vnd.oasis.opendocument.text";
	t["pptx"]	 = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	t["xlsx"]	 = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	t["docx"]	 = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	t["wmlc"]	 = "application/vnd.wap.wmlc";
	t["wasm"]	 = "application/wasm";
	t["7z"]		 = "application/x-7z-compressed";
	t["cco"]	 = "application/x-cocoa";
	t["jardiff"] = "application/x-java-archive-diff";
	t["jnlp"]	 = "application/x-java-jnlp-file";
	t["run"]	 = "application/x-makeself";
	t["pl"]		 = "application/x-perl";
	t["pm"]		 = "application/x-perl";
	t["prc"]	 = "application/x-pilot";
	t["pdb"]	 = "application/x-pilot";
	t["rar"]	 = "application/x-rar-compressed";
	t["rpm"]	 = "application/x-redhat-package-manager";
	t["sea"]	 = "application/x-sea";
	t["swf"]	 = "application/x-shockwave-flash";
	t["sit"]	 = "application/x-stuffit";
	t["tcl"]	 = "application/x-tcl";
	t["tk"]		 = "application/x-tcl";
	t["der"]	 = "application/x-x509-ca-cert";
	t["pem"]	 = "application/x-x509-ca-cert";
	t["crt"]	 = "application/x-x509-ca-cert";
	t["xpi"]	 = "application/x-xpinstall";
	t["xhtml"]	 = "application/xhtml+xml";
	t["xspf"]	 = "application/xspf+xml";
	t["zip"]	 = "application/zip";
	t["bin"]	 = "application/octet-stream";
	t["exe"]	 = "application/octet-stream";
	t["dll"]	 = "application/octet-stream";
	t["deb"]	 = "application/octet-stream";
	t["dmg"]	 = "application/octet-stream";
	t["iso"]	 = "application/octet-stream";
	t["img"]	 = "application/octet-stream";
	t["msi"]	 = "application/octet-stream";
	t["msp"]	 = "application/octet-stream";
	t["msm"]	 = "application/octet-stream";
	t["mid"]	 = "audio/midi";
	t["midi"]	 = "audio/midi";
	t["kar"]	 = "audio/midi";
	t["mp3"]	 = "audio/mpeg";
	t["ogg"]	 = "audio/ogg";
	t["m4a"]	 = "audio/x-m4a";
	t["ra"]		 = "audio/x-realaudio";
	t["3gpp"]	 = "video/3gpp";
	t["3gp"]	 = "video/3gpp";
	t["ts"]		 = "video/mp2t";
	t["mp4"]	 = "video/mp4";
	t["mpeg"]	 = "video/mpeg";
	t["mpg"]	 = "video/mpeg";
	t["mov"]	 = "video/quicktime";
	t["webm"]	 = "video/webm";
	t["flv"]	 = "video/x-flv";
	t["m4v"]	 = "video/x-m4v";
	t["mng"]	 = "video/x-mng";
	t["asx"]	 = "video/x-ms-asf";
	t["asf"]	 = "video/x-ms-asf";
	t["wmv"]	 = "video/x-ms-wmv";
	t["avi"]	 = "video/x-msvideo";
	return t;
}

std::string Mime::getType(const std::string& extension) {
	MimeIt it = _types.find(extension);
	if (it != _types.end())
		return it->second;
	return "application/octet-stream";
}

std::string Mime::getExtension(const std::string& mimeType) {
	for (MimeIt it = _types.begin(); it != _types.end(); ++it)
		if (it->second == mimeType)
			return it->first;
	return "";
}
