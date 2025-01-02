import zookeeper, { Exception } from 'node-zookeeper-client';

class qzookeeper {
    private client: zookeeper.Client;
    private zk_connection_string: string;
    private retry_attempts: number;
    private retry_interval: number;

    constructor(zk_connection_string: string, retry_attempts: number = 3, retry_interval: number = 1000) {
        this.zk_connection_string = zk_connection_string;
        this.client = zookeeper.createClient(zk_connection_string);
        this.retry_attempts = retry_attempts;
        this.retry_interval = retry_interval;

        // Add event listeners for various Zookeeper events
        this.client.on('connected', () => {
            console.log('qzookeeper client connected.');
        });

        this.client.on('disconnected', () => {
            console.log('qzookeeper client disconnected.');
        });

        this.client.on('auth_failed', () => {
            console.log('qzookeeper client authentication failed.');
        });

        this.client.on('session_expired', () => {
            console.log('qzookeeper session expired.');
            this.reconnect();
        });

        // this.client.on('error', (err: Error | Exception) => {
        //     // console.log(`Zookeeper client encountered an error: ${err.message}`);
        // });
    }

    connect(): void {
        this.client.connect();
        console.log('qzookeeper trying to connect');
    }

    private reconnect(): void {
        // Close the current client session and reconnect
        console.log('Closing current session...');
        this.client.close();
        
        // Recreate a new client instance and reconnect
        this.client = zookeeper.createClient(this.zk_connection_string);
        this.connect();
    }

    public get_data(zk_path: string, get_data_callback: (error: Error | null, data: Buffer) => void, default_value: string = '{}') {
        const attempt_get_data = (retry_count: number) => {
            if (retry_count <= this.retry_attempts) {
                this.client.getData(zk_path, (error: Error | Exception, data: Buffer, stat: zookeeper.Stat) => {
                    if (error) {
                        console.log(`Error fetching data (Attempt ${retry_count}):`, error);
                        setTimeout(() => attempt_get_data(retry_count + 1), this.retry_interval);
                    } else {
                        get_data_callback(null, data);
                    }
                });
            } else {
                console.log('Max retry attempts reached for get_data');
                get_data_callback(new Error('Max retry attempts reached'), Buffer.from(default_value));
            }
        };

        attempt_get_data(1);
    }

    close(): void {
        this.client.close();
        console.log('qzookeeper close');
    }
}

export default qzookeeper;


// const zkClient = new ZooKeeperClient('localhost:2181');
// zkClient.connect();

// zkClient.get_data('/my/path', (err, data) => {
//     if (err) {
//         console.error('Error:', err);
//     } else {
//         console.log('Data:', data.toString());
//     }
// });
