<!DOCTYPE html>

<html>
<head>
    <meta charset="utf-8">
    <title>1v1屏幕共享-推流端</title>

    <style type="text/css">
        body {
            margin: 0;
            padding: 0;
            font-family: Arial, sans-serif;
            font-size: 13px;
        }
        .highlight {
            background-color: #eeeeee;
            padding: 0.5em 1.5em;
            margin: 0 0 5px 0;
        }

        video {
            width: 480px;
            height: 360px;
            object-fit: contain;
        }

        button {
            background-color: #d84a38;
            border: none;
            border-radius: 2px;
            color: white ;
            margin: 5px 0 0 0;
            padding: 0.5em 0.7em 0.6em 0.7em;
            font-size: 16px;
            cursor: pointer;
        }

        button:hover {
            background-color: #cf402f;
        }
    </style>
</head>

<body>
    <h3>1v1屏幕共享-推流端</h3>

    <div class="highlight">
        推流端基本信息
        <span>
            uid={{.uid}}
            streamName={{.streamName}}
            audio={{.audio}}
            video={{.video}}
        </span>
    </div>
    <div>
        <video id="local_video" controls autoplay></video>
    </div>

    <button id="pushBtn">开始推流</button>
    <button id="stopPushBtn">停止推流</button>
    
    <input type="hidden" id="uid" value="{{.uid}}"> 
    <input type="hidden" id="streamName" value="{{.streamName}}"> 
    <input type="hidden" id="audio" value="{{.audio}}"> 
    <input type="hidden" id="video" value="{{.video}}"> 


    <script src="/static/js/adapter.js"></script>
    <script src="/static/js/jquery-2.1.1.min.js"></script>
    <script src="/static/js/push.js"></script>


</body>

</html>
