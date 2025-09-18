using System;
using System.Net.WebSockets;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace WebSocketClientExample
{
    public class AREvent
    {
        required public string EventType { get; set; }
        required public string UserId { get; set; }
        required public string SessionId { get; set; }
        required public long Timestamp { get; set; }
        required public object Data { get; set; }
    }

    public class SecureWebSocketClient : IDisposable
    {
        private readonly Uri _serverUri;
        private readonly string _userId;
        private readonly string _sessionId;
        private readonly string _deviceId;
        private readonly ClientWebSocket _client;
        private readonly X509Certificate2 _clientCert;
        private readonly CancellationTokenSource _cts = new();

        public event Action<string>? OnMessageReceived;
        public event Action? OnConnected;
        public event Action? OnDisconnected;
        public event Action<Exception>? OnError;

        public SecureWebSocketClient(string serverUrl, string userId, string sessionId = "", string? deviceId = null, string? clientCertPath = null, string? clientCertPassword = null)
        {
            _userId = userId;
            _sessionId = string.IsNullOrEmpty(sessionId) ? userId : sessionId;
            _deviceId = string.IsNullOrEmpty(deviceId) ? $"device_{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}" : deviceId;

            _serverUri = new Uri($"{serverUrl}?userId={_userId}&sessionId={_sessionId}&deviceId={_deviceId}");
            _client = new ClientWebSocket();

            if (!string.IsNullOrEmpty(clientCertPath))
            {
                _clientCert = new X509Certificate2(clientCertPath, clientCertPassword);
                _client.Options.ClientCertificates.Add(_clientCert);
            }
        }

        public async Task ConnectAsync()
        {
            try
            {
                await _client.ConnectAsync(_serverUri, _cts.Token);
                OnConnected?.Invoke();
                _ = ReceiveLoop(); // start receiving in background
            }
            catch (Exception ex)
            {
                OnError?.Invoke(ex);
            }
        }

        public async Task SendMessageAsync(string eventType, object data)
        {
            if (_client.State != WebSocketState.Open) return;

            AREvent arEvent = new()
            {
                EventType = eventType,
                UserId = _userId,      // server will overwrite
                SessionId = _sessionId,// server will overwrite
                Timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(),
                Data = data
            };

            string json = JsonSerializer.Serialize(arEvent);
            var bytes = Encoding.UTF8.GetBytes(json);
            await _client.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, _cts.Token);
        }

        private async Task ReceiveLoop()
        {
            var buffer = new byte[512 * 1024]; // 512KB buffer

            try
            {
                while (!_cts.Token.IsCancellationRequested && _client.State == WebSocketState.Open)
                {
                    var result = await _client.ReceiveAsync(new ArraySegment<byte>(buffer), _cts.Token);

                    if (result.MessageType == WebSocketMessageType.Close)
                    {
                        await _client.CloseAsync(WebSocketCloseStatus.NormalClosure, "Closed by client", CancellationToken.None);
                        OnDisconnected?.Invoke();
                        break;
                    }

                    string message = Encoding.UTF8.GetString(buffer, 0, result.Count);
                    OnMessageReceived?.Invoke(message);
                }
            }
            catch (Exception ex)
            {
                OnError?.Invoke(ex);
            }
        }

        public async Task DisconnectAsync()
        {
            _cts.Cancel();
            if (_client.State == WebSocketState.Open)
            {
                await _client.CloseAsync(WebSocketCloseStatus.NormalClosure, "Client closing", CancellationToken.None);
            }
            OnDisconnected?.Invoke();
        }

        public void Dispose()
        {
            _cts.Cancel();
            _client.Dispose();
        }
    }

    // Example usage
    class Program
    {
        static async Task Main(string[] args)
        {
            var wsClient = new SecureWebSocketClient(
                "wss://localhost:8443/ws",
                userId: "user123",
                sessionId: "session123",
                clientCertPath: "client-cert.pfx",
                clientCertPassword: "password"
            );

            wsClient.OnConnected += () => Console.WriteLine("Connected to server!");
            wsClient.OnMessageReceived += (msg) => Console.WriteLine($"Received: {msg}");
            wsClient.OnDisconnected += () => Console.WriteLine("Disconnected from server");
            wsClient.OnError += (ex) => Console.WriteLine($"Error: {ex.Message}");

            await wsClient.ConnectAsync();

            Console.WriteLine("Press Enter to send a test message");
            Console.ReadLine();

            await wsClient.SendMessageAsync("testEvent", new { content = "Hello from .NET client!" });

            Console.WriteLine("Press Enter to disconnect");
            Console.ReadLine();

            await wsClient.DisconnectAsync();
        }
    }
}
