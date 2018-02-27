# TODO: check for C++ and specific program dependencies

# install Ubuntu dependencies
if [ -z $(command -v google-chrome) ]
then
    echo 'Google Chrome was not found on this system. Installing...'
    wget -q -O - https://dl-ssl.google.com/linux/linux_signing_key.pub | sudo apt-key add -
    echo 'deb [arch=amd64] http://dl.google.com/linux/chrome/deb/ stable main' | sudo tee /etc/apt/sources.list.d/google-chrome.list
    sudo apt-get update 
    sudo apt-get install google-chrome-stable
fi

if [ -z $(command -v xdotool) ]
then
    echo 'xdotool was not found on this system. Installing...'
    sudo apt-get install xdotool
fi

if [ -z $(command -v scrot) ]
then
    echo 'scrot was not found on this system. Installing...'
    sudo apt-get install scrot
fi

echo 'System has all dependencies installed.'