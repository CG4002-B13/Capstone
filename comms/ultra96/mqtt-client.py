import aiomqtt


class MQTTClient:
    def __init__(
        self,
        broker_host="127.0.0.1",
        broker_port=1883,
        client_id=None,
        keepalive=60,
    ):
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.client_id = client_id
        self.keepalive = keepalive
        self._client = None
        self._task = None
        self._connected = False
        self._client = aiomqtt.Client(
            hostname=self.broker_host,
            port=self.broker_port,
            client_id=self.client_id,
            keepalive=self.keepalive,
        )

    async def connect(self):
        if self._connected:
            return

        await self._client.__aenter__()

    async def disconnect(self):
        """Disconnect from the MQTT broker."""
        if self._client:
            await self._client.__aexit__(None, None, None)
            self._client = None

    async def publish(
        self, topic: str, payload: str, qos: int = 0, retain: bool = False
    ):
        """Publish a message to a topic."""
        if not self._client:
            raise RuntimeError("Client not connected")
        await self._client.publish(topic, payload.encode(), qos=qos, retain=retain)

    async def subscribe(self, topic, callback, qos):
        """
        Subscribe to a topic and process incoming messages with the callback.

        Args:
            topic: Topic string.
            callback: Function called as callback(topic, payload).
            qos: Quality of Service level.
        """
        if not self._client:
            raise RuntimeError("Client not connected")

        async with self._client.messages() as messages:
            await self._client.subscribe(topic, qos=qos)
            async for message in messages:
                payload = message.payload.decode()
                await callback(message.topic.value, payload)


async def message_handler(topic: str, payload: str):
    print(f"[Message] {topic} => {payload}")
