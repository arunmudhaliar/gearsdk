// redis
import { Redis, RedisOptions } from "ioredis";
import { debug_print, LOG_LEVEL_0, LOG_LEVEL_2, LOG_LEVEL_4 } from "./sdktypes";

type redis_hash_iterator_callback = (field: string, value: string, arg?: any) => void;
type redis_scan_callback = (key: string, field: string, value: string, arg?: any) => void;

class qhiredis {
    private static __LOGTAG__: string = `qhiredis`;
    private redis_client: Redis | null = null;
    private readonly name: string;
    private readonly redis_ip: string;
    private readonly redis_port: number;
    private readonly redis_username?: string; // Optional username for Redis 6+
    private readonly redis_password?: string; // Optional password

    constructor(name: string, redis_ip: string, redis_port: number, redis_username?: string, redis_password?: string) {
        this.name = name;
        this.redis_ip = redis_ip;
        this.redis_port = redis_port;
        this.redis_username = redis_username;
        this.redis_password = redis_password;
    }

    async connect_redis(unix_socket = false): Promise<number> {
        if (this.redis_client) {
            debug_print(LOG_LEVEL_0, qhiredis.__LOGTAG__, `[${this.name}] Already connected to Redis at ${this.redis_ip}:${this.redis_port}`);
            return 0;
        }

        try {
            const options: RedisOptions = {
                host: this.redis_ip,
                port: this.redis_port,
                lazyConnect: true,
                connectTimeout: 30500, // 30.5 seconds
                // Authentication options
                username: this.redis_username, // Optional: If you're using Redis ACL (6.x and later)
                password: this.redis_password, // If password is set
                retryStrategy: times => Math.min(times * 200, 5000),
                maxRetriesPerRequest: 5,  // or null to disable per-request retries
                enableOfflineQueue: false
            };

            if (unix_socket) {
                options.path = this.redis_ip;
                delete options.host;
                delete options.port;
            }

            this.redis_client = new Redis(options);
            /*
            // This callbacks not seems to be getting called. Need to investigate.
            this.redis_client.on('error', (err) => {
                console.error(`[${this.name}] Redis client error:`, err);
            });
            this.redis_client.on('close', () => {
                console.warn(`[${this.name}] Redis connection closed.`);
            });
            this.redis_client.on('end', () => {
                console.warn(`[${this.name}] Redis connection ended.`);
            });
            this.redis_client.on('reconnecting', () => {
                console.warn(`[${this.name}] Redis reconnecting...`);
            });
            */
            debug_print(LOG_LEVEL_4, qhiredis.__LOGTAG__, `[${this.name}] Tyring connection to Redis at ${this.redis_ip}:${this.redis_port}`);
            await this.redis_client.connect();
            debug_print(LOG_LEVEL_0, qhiredis.__LOGTAG__, `[${this.name}] Connected to Redis at ${this.redis_ip}:${this.redis_port}`);
            // // Perform authentication if username and password are provided
            // if (this.redis_username && this.redis_password) {
            //     debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `[${this.name}] Tyring authentication`);
            //     await this.redis_client.auth(this.redis_username, this.redis_password);
            //     debug_print(LOG_LEVEL_4, userserver.__LOGTAG__, `[${this.name}] Authenticated to Redis with username and password`);
            // }

            this.redis_client.config("SET", "notify-keyspace-events", "Ex");
            return 0;
        } catch (error) {
            console.error(`[${this.name}] Failed to connect to Redis:`, error);
            this.disconnect_redis();
            return 1;
        }
    }

    disconnect_redis(): void {
        if (this.redis_client) {
            this.redis_client.quit();
            this.redis_client = null;
            debug_print(LOG_LEVEL_0, qhiredis.__LOGTAG__, `[${this.name}] Redis connection closed`);
        }
    }

    async retry_connection(): Promise<number> {
        debug_print(LOG_LEVEL_0, qhiredis.__LOGTAG__, `[${this.name}] Retrying Redis connection...`);
        this.disconnect_redis();
        return this.connect_redis();
    }

    async set_value(key: string, value: string, expiry_in_sec?: number): Promise<number> {
        if (!this.redis_client) return 2;

        try {
            if (expiry_in_sec) {
                await this.redis_client.set(key, value, "EX", expiry_in_sec);
            } else {
                await this.redis_client.set(key, value);
            }
            return 0;
        } catch (error) {
            console.error(`[${this.name}] Redis SET error:`, error);
            await this.retry_connection();
            return 1;
        }
    }

    async get_value(key: string): Promise<string | null> {
        if (!this.redis_client) return null;

        try {
            return await this.redis_client.get(key);
        } catch (error) {
            console.error(`[${this.name}] Redis GET error:`, error);
            await this.retry_connection();
            return null;
        }
    }

    async set_hash_value(hash_key: string, field: string, value: string): Promise<number> {
        if (!this.redis_client) return 2;

        try {
            const result = await this.redis_client.hset(hash_key, field, value);
            debug_print(LOG_LEVEL_4, qhiredis.__LOGTAG__, `[${this.name}] HSET result for ${hash_key}:${field}: ${result}`);
            return 0;
        } catch (error) {
            console.error(`[${this.name}] Redis HSET error:`, error);
            await this.retry_connection();
            return 1;
        }
    }

    async get_hash_value(hash_key: string, field: string): Promise<string | null> {
        if (!this.redis_client) return null;

        try {
            return await this.redis_client.hget(hash_key, field);
        } catch (error) {
            console.error(`[${this.name}] Redis HGET error:`, error);
            await this.retry_connection();
            return null;
        }
    }

    async delete_hash_field(hash_key: string, field: string): Promise<number> {
        if (!this.redis_client) return 2;

        try {
            const result = await this.redis_client.hdel(hash_key, field);
            if (result === 1) {
                debug_print(LOG_LEVEL_4, qhiredis.__LOGTAG__, `[${this.name}] Deleted field ${field} from hash ${hash_key}`);
            } else {
                console.warn(`[${this.name}] Field ${field} not found in hash ${hash_key}`);
            }
            return 0;
        } catch (error) {
            console.error(`[${this.name}] Redis HDEL error:`, error);
            return 1;
        }
    }

    async iterate_hash(hash_key: string, callback: redis_hash_iterator_callback, arg?: any): Promise<void> {
        if (!this.redis_client) {
            console.error(`[${this.name}] Redis client is null`);
            return;
        }

        const cursor = 0;
        while (this.redis_client != null) {
            const [new_cursor, results] = await this.redis_client.hscan(hash_key, cursor);
            for (let i = 0; i < results.length; i += 2) {
                callback(results[i], results[i + 1], arg);
            }
            if (new_cursor === "0") break;
        }
    }

    async scan(prefix_key: string, callback: redis_scan_callback, arg?: any): Promise<void> {
        if (!this.redis_client) {
            console.error(`[${this.name}] Redis client is null`);
            return;
        }

        let cursor = "0";
        do {
            const [new_cursor, keys] = await this.redis_client.scan(cursor, "MATCH", `${prefix_key}:*`);
            for (const key of keys) {
                if (this.redis_client == null) {
                    console.error(`[${this.name}] Redis client is null`);
                    return;
                }
                const hash_content = await this.redis_client.hgetall(key);
                for (const [field, value] of Object.entries(hash_content)) {
                    callback(key, field, value, arg);
                }
            }
            cursor = new_cursor;
        } while (cursor !== "0" && this.redis_client != null);
    }

    async incr(key: string): Promise<number | null> {
        if (!this.redis_client) return null;

        try {
            return await this.redis_client.incr(key);
        } catch (error) {
            console.error(`[${this.name}] Redis INCR error:`, error);
            await this.retry_connection();
            return null;
        }
    }

    async incr_by(key: string, increment: number): Promise<number | null> {
        if (!this.redis_client) return null;

        try {
            return await this.redis_client.incrby(key, increment);
        } catch (error) {
            console.error(`[${this.name}] Redis INCRBY error:`, error);
            await this.retry_connection();
            return null;
        }
    }

    async decr_by(key: string, decrement: number): Promise<number | null> {
        if (!this.redis_client) return null;

        try {
            return await this.redis_client.decrby(key, decrement);
        } catch (error) {
            console.error(`[${this.name}] Redis DECRBY error:`, error);
            await this.retry_connection();
            return null;
        }
    }
}

export default qhiredis;

// Usage with username and password
// const redisClient = new qhiredis("exampleClient", "127.0.0.1", 6379, "my-username", "my-password");
// await redisClient.connect_redis();