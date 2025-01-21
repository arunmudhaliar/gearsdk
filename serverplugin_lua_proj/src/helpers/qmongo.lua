local mongodb = require("mongorover") -- Assuming a Lua MongoDB driver exists

local qmongo = {}
qmongo.__index = qmongo
qmongo.__LOGTAG__ = "qmongo"

-- Constructor
function qmongo:new(appName, dbName, uri, errorHandler)
    if type(appName) ~= "string" or type(dbName) ~= "string" or type(uri) ~= "string" then
        error("appName, dbName, and uri must all be strings")
    end

    if errorHandler and type(errorHandler) ~= "function" then
        error("errorHandler must be a function")
    end
    -- Initialize instance
    local self = setmetatable({}, qmongo)
    self.appName = appName
    self.dbName = dbName
    self.uri = uri
    self.errorHandler = errorHandler or nil
    self.client = nil
    self.database = nil
    self.collections = {}

    return self
    -- local obj = {
    --     appName = appName,
    --     dbName = dbName,
    --     uri = uri,
    --     client = nil,
    --     database = nil,
    --     collections = {},
    --     errorHandler = errorHandler or nil
    -- }
    -- setmetatable(obj, qmongo)
    -- return obj
end

-- Error handler
function qmongo:handleError(error)
    if self.errorHandler then
        self.errorHandler(error)
    else
        print(error)
    end
end

-- Connect to MongoDB
function qmongo:connect()
    local success, err = pcall(function()
        self.client = mongodb.MongoClient.new(self.uri, { appName = self.appName, serverSelectionTimeoutMS = 30000 })
        print(string.format("%s: trying to connect to MongoDB %s, appName %s", self.__LOGTAG__, self.uri, self.appName))
        -- self.client:connect()    -- not required for mongover
        self.database = self.client:getDatabase(self.dbName)
        print(string.format("%s: Connected to MongoDB: %s", self.__LOGTAG__, self.uri))
    end)
    if not success then
        self:handleError(err)
        error(err)
    end
end

-- Disconnect from MongoDB
function qmongo:disconnect()
    if self.client then
        self.client:close()
        self.client = nil
        self.database = nil
        self.collections = {}
        print(string.format("%s: Disconnected from MongoDB", self.__LOGTAG__))
    end
end

-- Get or create collection
function qmongo:getCollection(collectionName)
    if not self.database then
        print("Database connection is not initialized.")
        return nil
    end

    if not self.collections[collectionName] then
        local collection = self.database:getCollection(collectionName)
        self.collections[collectionName] = collection
    end

    return self.collections[collectionName]
end

-- Insert a document
function qmongo:insert(collectionName, document)
    local success, err = pcall(function()
        local collection = self:getCollection(collectionName)
        if not collection then error(string.format("Collection '%s' not found.", collectionName)) end
        collection:insertOne(document)
        print(string.format("%s: Document inserted into %s", self.__LOGTAG__, collectionName))
    end)
    if not success then
        self:handleError(err)
        error(err)
    end
end

-- Update a document
function qmongo:update(collectionName, filter, update)
    local success, err = pcall(function()
        local collection = self:getCollection(collectionName)
        if not collection then error(string.format("Collection '%s' not found.", collectionName)) end
        local result = collection:findOneAndUpdate(filter, { ["$set"] = update }, { upsert = true, returnDocument = "after" })
        if not result then error("Update failed or document not found.") end
        print(string.format("%s: Document updated in %s", self.__LOGTAG__, collectionName))
    end)
    if not success then
        self:handleError(err)
        error(err)
    end
end

-- Find documents
function qmongo:find(collectionName, filter)
    local success, documents = pcall(function()
        local collection = self:getCollection(collectionName)
        if not collection then error(string.format("Collection '%s' not found.", collectionName)) end
        return collection:find(filter):toArray()
    end)
    if not success then
        self:handleError(documents)
        error(documents)
    end
    print(string.format("%s: Found %d documents in %s", self.__LOGTAG__, #documents, collectionName))
    return documents
end

-- Find and upsert
function qmongo:find_and_upsert(collectionName, find_query_cb, update_query_cb, insert_query_cb)
    local success, result = pcall(function()
        local collection = self:getCollection(collectionName)
        if not collection then error(string.format("Collection '%s' not found.", collectionName)) end

        local find_query = {}
        find_query_cb(find_query)

        local update_doc = {}
        update_query_cb(update_doc)

        local set_on_insert = {}
        insert_query_cb(set_on_insert)

        local update_ops = {
            ["$set"] = update_doc,
            ["$setOnInsert"] = set_on_insert
        }
        -- if type(collection.update_one) == "function" then
        --     print(string.format("%s: update_one Found", self.__LOGTAG__))
        -- end
        return collection:update_one(find_query, update_ops, true)
    end)
    if not success then
        self:handleError(result)
        error(result)
    end
    return 0 -- Indicating success
end

-- Create index if not exists
function qmongo:createClientIndexIfNot(collectionName, indexKey)
    local success, err = pcall(function()
        local collection = self:getCollection(collectionName)
        if not collection then error(string.format("Collection '%s' not found.", collectionName)) end

        local indexExists = collection:indexExists(indexKey)
        if not indexExists then
            collection:createIndex(indexKey)
            print(string.format("%s: Index created for %s", self.__LOGTAG__, collectionName))
        else
            print(string.format("%s: Index already exists for %s", self.__LOGTAG__, collectionName))
        end
    end)
    if not success then
        self:handleError(err)
        error(err)
    end
end

return qmongo
