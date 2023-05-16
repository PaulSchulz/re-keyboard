--- venv/lib/python3.11/site-packages/toltec/bash.py	2023-05-16 21:37:59.741510526 +0930
+++ venv/lib/python3.11/site-packages/toltec/bash.py.new	2023-05-16 21:37:48.701325541 +0930
@@ -218,7 +218,10 @@
         index = int(lexer.get_token())
         assert lexer.get_token() == "]"
         assert lexer.get_token() == "="
-        value = _parse_string(lexer.get_token())
+        string_token = lexer.get_token()
+        if string_token == "$":
+            string_token = lexer.get_token()
+        value = _parse_string(string_token)
 
         # Grow the result array so that the index exists
         if index >= len(result):
@@ -258,7 +261,10 @@
         key = lexer.get_token()
         assert lexer.get_token() == "]"
         assert lexer.get_token() == "="
-        value = _parse_string(lexer.get_token())
+        string_token = lexer.get_token()
+        if string_token == "$":
+            string_token = lexer.get_token()
+        value = _parse_string(string_token)
 
         result[key] = value
 
@@ -296,7 +302,10 @@
         elif "A" in var_flags:
             var_value = _parse_assoc(lexer)
         else:
-            var_value = _parse_string(lexer.get_token())
+            string_token = lexer.get_token()
+            if string_token == "$":
+                string_token = lexer.get_token()
+            var_value = _parse_string(string_token)
     else:
         lexer.push_token(lookahead)
 
