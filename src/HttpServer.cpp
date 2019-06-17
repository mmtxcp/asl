// Copyright(c) 1999-2019 aslze
// Licensed under the MIT License (http://opensource.org/licenses/MIT)

#include <asl/Socket.h>
#include <asl/Map.h>
#include <asl/File.h>
#include <asl/IniFile.h>
#include <asl/SocketServer.h>
#include <asl/HttpServer.h>
#include <asl/WebSocket.h>

namespace asl {

bool verbose = false;

HttpServer::HttpServer(int port)
{
	_requestStop = false;
	_proto = "HTTP/1.1";
	_methods = "GET, POST, OPTIONS, PUT, DELETE, PATCH, HEAD";
	if (port >= 0)
		bind(port);
	_wsserver = NULL;
	_cors = false;
	_mimetypes = split(
		"css:text/css,"
		"gif:image/gif,"
		"htm:text/html,"
		"html:text/html,"
		"jpeg:image/jpeg,"
		"jpg:image/jpeg,"
		"js:application/javascript,"
		"json:application/json,"
		"png:image/png,"
		"txt:text/plain,"
		"xml:text/xml", ',', ':');
}

void HttpServer::addMimeType(const String& ext, const String& type)
{
	_mimetypes[ext] = type;
}

void HttpServer::serve(Socket client)
{
	while(client.waitInput())
	{
		if(client.disconnected())
			break;

		HttpRequest request(client);
		if (client.error())
			break;

		if (request.header("Upgrade") == "websocket" && _wsserver)
		{
			if(verbose) printf("handing over to ws\n");
			_wsserver->process(client, request.headers());
			return;
		}
		HttpResponse response(request, _proto);
		response.put("");
		if (_cors && request.hasHeader("Origin"))
		{
			response.setHeader("Access-Control-Allow-Origin", "*");
		}
		if (!handleOptions(request, response))
		{
			serve(request, response);
			if (response.code() == 405)
				response.setHeader("Allow", _methods);

			if (response.containsFile())
			{
				File file((String)response.body());
				if (!file.exists())
				{
					response.setCode(404);
					response.setHeader("Content-Type", "text/html");
					response.put("<h1>Error</h1><p>File <b>" + file.name() + "</b> not found</p>");
					continue;
				}

				String mime = _mimetypes.get(file.extension(), "text/plain");
				response.setHeader("Date", Date::now().toString(Date::HTTP));
				response.setHeader("Content-Type", mime);
				if (!response.hasHeader("Cache-Control"))
					response.setHeader("Cache-Control", "max-age=60, public");
				response.putFile(file.path());
			}
			else
				response.write();
		}
		
		if(_proto=="HTTP/1.0" || request.header("Connection") == "close")
			break;
	}
}

void HttpServer::setRoot(const String& root)
{
	_webroot = root;
}

void HttpServer::serveFile(HttpRequest& request, HttpResponse& response)
{
	if (request.method() == "GET")
	{
		String path = request.path();

		if (path.endsWith("/"))
			path += "index.html";

		String localpath = _webroot + path;
		File file(localpath);
		if (file.isDirectory())
		{
			response.setCode(301);
			response.setHeader("Location", "http://" + request.header("Host") + path+'/');
		}
		else if (file.exists())
		{
			if (request.hasHeader("If-Modified-Since"))
			{
				Date ifdate = request.header("If-Modified-Since");
				if (file.lastModified() <= ifdate + 1.0) {
					response.setCode(304);
					return;
				}
			}
			response.setHeader("Last-Modified", file.lastModified().toString(Date::HTTP));
			response.put(file);
		}
		else
		{
			response.setCode(404);
			response.setHeader("Content-Type", "text/html");
			response.put("<h1>Not found</h1>");
		}
	}
	else
	{
		response.setCode(501);
		response.setHeader("Content-Type", "text/html");
		response.put("<h1>Not implemented</h1>");
	}
}

void HttpServer::addMethod(const String& verb)
{
	if (_methods == "")
		_methods = "GET, POST, OPTIONS, PUT, DELETE, PATCH, HEAD";
	Array<String> methods = _methods.split(", ");
	if (!methods.contains(verb))
		methods << verb;
	_methods = methods.join(", ");
}

bool HttpServer::handleOptions(HttpRequest& request, HttpResponse& response)
{
	if (request.method() == "OPTIONS")
	{
		if (request.hasHeader("Origin"))
			response.setHeader("Access-Control-Allow-Methods", _methods);
		if (request.hasHeader("Access-Control-Request-Headers"))
			response.setHeader("Access-Control-Allow-Headers", request.header("Access-Control-Request-Headers"));
		response.setHeader("Allow", _methods);
		response.setHeader("Content-Length", "0");
		response.setCode(200);
		response.write();
		return true;
	}
	else
		return false;
}

void HttpServer::handle(HttpRequest& request, HttpResponse& response)
{
	if (_webroot)
		serveFile(request, response);
}

}
