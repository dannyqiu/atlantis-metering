import http.server
import socketserver

class RequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        print(post_data.decode('utf-8'))
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(b'POST request received')

PORT = 8080

with socketserver.TCPServer(("", PORT), RequestHandler) as httpd:
    print("serving at port", PORT)
    httpd.allow_reuse_address = True
    httpd.serve_forever()
