wget https://github.com/github/hub/releases/download/v2.2.1/hub-linux-amd64-2.2.1.tar.gz
tar xvfz hub-linux-amd64-2.2.1.tar.gz
sudo mv hub-linux-amd64-2.2.1/hub /usr/local/bin/
sudo chmod +x /usr/local/bin/hub


cat > ~/.config/hub << EOF
github.com:
- user: ${HUB_USER}
  oauth_token: ${HUB_TOKEN}
  protocol: https
EOF

message=`git log -1 --pretty=%B`

echo "Automatic translation import" | hub pull-request -b trunk -m "$message" -F -
