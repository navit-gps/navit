var returnPath=new Array();

function load(page) {
    gui.pushPage(page);
    pageLoader.source=page;
    cellar.onStartup(page);
}

function back() {
    var page=gui.popPage();
    console.log(page);
    pageLoader.source=page;
    cellar.onStartup(page);
}

