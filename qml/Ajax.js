
.pragma library // single-instance

function doRequest (method, url, onFinished, userData) {
    var ajax = new XMLHttpRequest ();
    ajax.onreadystatechange = function () {
        switch (ajax.readyState) {
        case XMLHttpRequest.UNSENT:
            //console.log ("Ajax.doRequest :", method,  url, "state unsent");
            break;
        case XMLHttpRequest.OPENED:
            //console.log ("Ajax.doRequest :", method,  url, "state opened");
            break;
        case XMLHttpRequest.HEADERS_RECEIVED:
            //console.log ("Ajax.doRequest :", method,  url, "state headers received");
            //console.debug (ajax.getAllResponseHeaders ());
            break;
        case XMLHttpRequest.LOADING:
            //console.log ("Ajax.doRequest :", method,  url, "state loading...");
            break;
        case XMLHttpRequest.DONE:
            //console.log ("Ajax.doRequest :", method,  url, "state done !");
            if (ajax.status === 200) {
                onFinished (JSON.parse (ajax.responseText));
            }
            else { console.log ("Ajax.doRequest :", method,  url, "status not OK !!!", ajax.status, ajax.statusText); }
            break;
        default:
            break;
        }
    }
    //console.log ("Ajax.doRequest url=", url);
    ajax.open ((method || 'GET'), url);
    ajax.setRequestHeader ("Accept", "application/json");
    ajax.setRequestHeader ("Accept-encoding", "gzip,deflate");
    ajax.setRequestHeader ("Accept-charset", "utf-8");
    ajax.send ();
}
