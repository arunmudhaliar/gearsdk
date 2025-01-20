#![allow(non_camel_case_types)]  // Disable the warning for non-CamelCase types
use mongodb::{bson::{doc, Document}, options::IndexOptions, Client, Collection, Database, IndexModel};
use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use std::error::Error;
use tokio;

pub struct qmongo {
    app_name: String,
    db_name: String,
    uri_string: String,
    client: Option<Client>,
    database: Option<Database>,
    collections: HashMap<u64, Collection<Document>>, // Use `Collection<Document>`
}

impl qmongo {
    pub fn new(app_name: String, db_name: String, uri_string: String) -> Self {
        qmongo {
            app_name,
            db_name,
            uri_string,
            client: None,
            database: None,
            collections: HashMap::new(),
        }
    }

    pub async fn connect(&mut self) -> Result<(), Box<dyn Error>> {
        let client = Client::with_uri_str(&self.uri_string).await?;
        self.client = Some(client.clone());

        let database = client.database(&self.db_name);
        self.database = Some(database.clone());

        println!("Connected to MongoDB at {}", &self.uri_string);

        Ok(())
    }

    pub fn cleanup(&mut self) {
        // Cleanup resources here
        self.collections.clear();
        self.database = None;
        self.client = None;
    }

    pub async fn create_client_index_if_not(&mut self, collection_name: &str) -> Result<(), Box<dyn Error>> {
        let collection = self.get_collection(collection_name).await?;
    
        // Create an IndexModel for the "field_name" index
        let index_model = IndexModel::builder()
            .keys(doc! { "field_name": 1 })  // Index key with ascending order
            .options(IndexOptions::builder().unique(true).build())  // Options, such as unique index
            .build();
    
        // Create the index on the collection
        collection.create_index(index_model).await?;
    
        Ok(())
    }

    pub async fn find_and_upsert(
        &mut self,
        collection_name: &str,
        find_query: Document,
        update_query: Document,
        insert_query: Document,
    ) -> Result<(), Box<dyn Error>> {
        let collection = self.get_collection(collection_name).await?;

        // Perform the upsert operation
        collection.update_one(
            find_query,
            doc! {
                "$set": update_query,
                "$setOnInsert": insert_query
            },
        ).await?;

        Ok(())
    }

    pub async fn find_and_update(
        &mut self,
        collection_name: &str,
        filter: Document,
        update_doc: Document,
    ) -> Result<(), Box<dyn Error>> {
        let collection = self.get_collection(collection_name).await?;

        collection.update_one(filter, doc! { "$set": update_doc }).await?;

        Ok(())
    }

    pub async fn insert(&mut self, collection_name: &str, query: Document) -> Result<(), Box<dyn Error>> {
        let collection = self.get_collection(collection_name).await?;
        collection.insert_one(query).await?;

        Ok(())
    }

    pub async fn update(&mut self, collection_name: &str, query: Document, update: Document) -> Result<(), Box<dyn Error>> {
        let collection = self.get_collection(collection_name).await?;

        collection.update_one(query, doc! { "$set": update }).await?;

        Ok(())
    }

    // pub async fn find(
    //     &self,
    //     collection_name: &str,
    //     query: Document,
    // ) -> Result<Vec<Document>, Box<dyn Error>> {
    //     let collection = self.get_collection(collection_name).await?;
        
    //     let mut cursor = collection.find(query).await?;
    //     let mut documents = Vec::new();
    
    //     while cursor.advance().await? {
    //         if let Some(raw_doc) = cursor.current() {
    //             // Convert RawDocument to Document using the try_from method
    //             let document = Document::try_from(raw_doc)?;
    //             documents.push(document);
    //         }
    //     }
    
    //     Ok(documents)
    // }

    async fn get_collection(&mut self, collection_name: &str) -> Result<Collection<Document>, Box<dyn Error>> { 
        // Make self mutable since we're modifying collections
        if let Some(database) = &self.database {
            let crc = self.calculate_crc(collection_name);
    
            if let Some(collection) = self.collections.get(&crc) {
                return Ok(collection.clone());
            }
    
            let collection = database.collection(collection_name);
            self.collections.insert(crc, collection.clone()); // Now we can mutate self.collections
    
            Ok(collection)
        } else {
            Err("Database is not initialized.".into())
        }
    }

    fn calculate_crc(&self, collection_name: &str) -> u64 {
        let crc = crc32fast::hash(collection_name.as_bytes());
        crc as u64
    }
}

/* 
fn main() -> Result<(), Box<dyn Error>> {
    let app_name = "app_name".to_string();
    let db_name = "db_name".to_string();
    let uri_string = "mongodb://localhost:27017".to_string();

    let mut qmongo = qmongo::new(app_name, db_name, uri_string);

    tokio::runtime::Runtime::new()?.block_on(async {
        qmongo.connect().await?;

        let collection_name = "collection_name";

        // Example of inserting a document
        let insert_query = doc! { "hello": "world" };
        qmongo.insert(collection_name, insert_query).await?;

        // Example of finding documents
        let find_query = doc! { "hello": "world" };
        let documents = qmongo.find(collection_name, find_query).await?;

        for doc in documents {
            println!("{:?}", doc);
        }

        qmongo.cleanup();
    });

    Ok(())
}
*/
