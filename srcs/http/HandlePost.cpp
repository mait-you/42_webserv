#include "../../includes/Response.hpp"

static bool locationSupportsUpload(LocationConfig* locConfig) {
	if (!locConfig)
		return false;
	if (!locConfig->upload)
		return false;
	if (locConfig->upload_path.empty())
		return false;
	return true;
}

void Response::handlePost(Request& , ServerConfig&srv , LocationConfig* locConfig) {
	if (!locationSupportsUpload(locConfig)) {
		errorPage(srv, locConfig, 405, "Method Not Allowed");
		return;
	}
}
