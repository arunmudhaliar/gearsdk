import { MongoClient, Db, Collection, Document } from "mongodb";
import { debug_error, debug_print, LOG_LEVEL_0, LOG_LEVEL_2, LOG_LEVEL_4 } from "./sdktypes";

type ErrorHandler = (error: Error) => void;
type type_qmongo_findupdate_find_query_cb = (findQuery: Document) => void;
type type_qmongo_findupdate_update_query_cb = (updateQuery: Document) => void;
type type_qmongo_findupdate_insert_query_cb = (insertQuery: Document) => void;

export class qmongo {
    private static __LOGTAG__: string = `qmongo`;
    private appName: string;
    private dbName: string;
    private uri: string;
    private client: MongoClient | null = null;
    private database: Db | null = null;
    private collections: Map<string, Collection> = new Map();
    private errorHandler: ErrorHandler | null = null;

    constructor(appName: string, dbName: string, uri: string, errorHandler?: ErrorHandler) {
        this.appName = appName;
        this.dbName = dbName;
        this.uri = uri;
        if (errorHandler) this.errorHandler = errorHandler;
    }

    private handleError(error: Error) {
        if (this.errorHandler) {
            this.errorHandler(error);
        } else {
            console.error(error.message);
        }
    }

    async connect(): Promise<void> {
        try {
            this.client = new MongoClient(this.uri, { appName: this.appName, serverSelectionTimeoutMS: 30000});
            debug_print(LOG_LEVEL_4, qmongo.__LOGTAG__, `trying to connect mongo ${this.uri}, appName ${this.appName}`);
            await this.client.connect();
            this.database = this.client.db(this.dbName);
            debug_print(LOG_LEVEL_0, qmongo.__LOGTAG__, `Connected to MongoDB: ${this.uri}`);
        } catch (error) {
            this.handleError(error as Error);
            throw error;
        }
    }

    async disconnect(): Promise<void> {
        if (this.client) {
            await this.client.close();
            this.client = null;
            this.database = null;
            this.collections.clear();
            debug_print(LOG_LEVEL_0, qmongo.__LOGTAG__, "Disconnected from MongoDB");
        }
    }

    private getCollection(collectionName: string): Collection | null {
        if (!this.database) {
            console.error("Database connection is not initialized.");
            return null;
        }

        if (!this.collections.has(collectionName)) {
            const collection = this.database.collection(collectionName);
            this.collections.set(collectionName, collection);
        }

        return this.collections.get(collectionName) || null;
    }

    async insert(collectionName: string, document: Document): Promise<void> {
        try {
            const collection = this.getCollection(collectionName);
            if (!collection) throw new Error(`Collection "${collectionName}" not found.`);

            await collection.insertOne(document);
            debug_print(LOG_LEVEL_4, qmongo.__LOGTAG__, `Document inserted into ${collectionName}`);
        } catch (error) {
            this.handleError(error as Error);
            throw error;
        }
    }

    async update(collectionName: string, filter: Document, update: Document): Promise<void> {
        try {
            const collection = this.getCollection(collectionName);
            if (!collection) throw new Error(`Collection "${collectionName}" not found.`);

            const result = await collection.findOneAndUpdate(filter, { $set: update }, { upsert: true, returnDocument: "after" });
            if (!result || !result.value) throw new Error("Update failed or document not found.");
            debug_print(LOG_LEVEL_4, qmongo.__LOGTAG__, `Document updated in ${collectionName}`);
        } catch (error) {
            this.handleError(error as Error);
            throw error;
        }
    }

    async find(collectionName: string, filter: Document): Promise<Document[]> {
        try {
            const collection = this.getCollection(collectionName);
            if (!collection) throw new Error(`Collection "${collectionName}" not found.`);

            const documents = await collection.find(filter).toArray();
            debug_print(LOG_LEVEL_4, qmongo.__LOGTAG__, `Found ${documents.length} documents in ${collectionName}`);
            return documents;
        } catch (error) {
            this.handleError(error as Error);
            throw error;
        }
    }

    async find_and_upsert(
        collection_name: string,
        find_query_cb: type_qmongo_findupdate_find_query_cb,
        update_query_cb: type_qmongo_findupdate_update_query_cb,
        insert_query_cb: type_qmongo_findupdate_insert_query_cb
    ): Promise<number> {
        try {
            const collection = this.getCollection(collection_name);
            if (!collection) throw new Error(`Collection "${collection_name}" not found.`);

            const find_query: Document = {};
            find_query_cb(find_query);

            const update_doc: Document = {};
            const set_on_insert: Document = {};

            // Add $set to update
            update_query_cb(update_doc);

            // Add $setOnInsert to insert document if not found
            insert_query_cb(set_on_insert);

            const update_ops: Document = {
                $set: update_doc,
                $setOnInsert: set_on_insert,
            };

            // Perform the update (upsert)
            const result = await collection.updateOne(find_query, update_ops, { upsert: true });
            if (result.modifiedCount > 0 || result.upsertedCount > 0) {
                debug_print(LOG_LEVEL_4, qmongo.__LOGTAG__, `Upsert successful for ${collection_name}`);
                return 0;
            } else if (result.matchedCount > 0 && result.modifiedCount === 0){
                debug_print(LOG_LEVEL_4, qmongo.__LOGTAG__, `No change in the fields ${collection_name}`);
                return 0;
            } else {
                debug_error(qmongo.__LOGTAG__, `No document was updated or inserted in ${collection_name}`);
                return 1;
            }
        } catch (error) {
            this.handleError(error as Error);
            throw error;
        }
    }

    async createClientIndexIfNot(collectionName: string, indexKey: Document): Promise<void> {
        try {
            const collection = this.getCollection(collectionName);
            if (!collection) throw new Error(`Collection "${collectionName}" not found.`);

            const indexExists = await collection.indexExists(Object.keys(indexKey));
            if (!indexExists) {
                await collection.createIndex(indexKey);
                debug_print(LOG_LEVEL_4, qmongo.__LOGTAG__, `Index created for ${collectionName}`);
            } else {
                debug_print(LOG_LEVEL_4, qmongo.__LOGTAG__, `Index already exists for ${collectionName}`);
            }
        } catch (error) {
            this.handleError(error as Error);
            throw error;
        }
    }
}
