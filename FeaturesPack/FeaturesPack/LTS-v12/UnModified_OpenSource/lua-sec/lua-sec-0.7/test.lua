rc=os.execute("sh test.sh")
if rc == true then os.exit(0) elseif rc== 0 then os.exit(0) else os.exit(1); end
