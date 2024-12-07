-- Postgresql db setup

/*
apt update
postgresql -v
apt install postgresql-17-partman
ls /usr/share/postgresql/17/extension/pg_partman.control
*/

DROP EXTENSION IF EXISTS pg_partman CASCADE;
CREATE EXTENSION pg_partman WITH SCHEMA partman;
GRANT USAGE ON SCHEMA partman TO postgres;

-- Drop all table 
DO $$
DECLARE
    table_names text[] := ARRAY[
        'flush_egress',
        'validate_token', 
        'parse',
        'recv_cb'
    ];
    current_table text;
BEGIN
    FOREACH current_table IN ARRAY table_names
    LOOP
        -- Delete partman configuration entry if it exists
        EXECUTE format(
            'DELETE FROM partman.part_config 
            WHERE parent_table = ''gsdk_stats.stats_count_%I'' ', 
            current_table
        );

        -- Drop the table if it exists
        EXECUTE format(
            'DROP TABLE IF EXISTS gsdk_stats.stats_count_%I CASCADE', 
            current_table
        );
    END LOOP;
END $$;


-- TRUNCATE all table 
DO $$
DECLARE
    table_names text[] := ARRAY[
        'flush_egress',
        'validate_token', 
        'parse',
        'recv_cb'
    ];
    current_table text;
    table_exists boolean;
BEGIN
    FOREACH current_table IN ARRAY table_names
    LOOP
        -- Check if the table exists
        SELECT EXISTS (
            SELECT 1 FROM information_schema.tables
            WHERE table_schema = 'gsdk_stats'
            AND table_name = format('stats_count_%I', current_table)
        ) INTO table_exists;

        -- Truncate the table if it exists
        IF table_exists THEN
            EXECUTE format('TRUNCATE TABLE gsdk_stats.stats_count_%I', current_table);
        END IF;
    END LOOP;
END $$;


-- Create tables
DO $$
DECLARE
    table_names text[] := ARRAY['flush_egress',
								'validate_token', 
								'parse',
								'recv_cb'
							   ];
    current_table text;
BEGIN
    FOREACH current_table IN ARRAY table_names
    LOOP
        -- Create the table
        EXECUTE format('CREATE TABLE IF NOT EXISTS gsdk_stats.stats_count_%I
        (
            count_val bigint,
            session text COLLATE pg_catalog."default",
            pid text COLLATE pg_catalog."default",
            version text COLLATE pg_catalog."default",
            epic text COLLATE pg_catalog."default",
            myth text COLLATE pg_catalog."default",
            legend text COLLATE pg_catalog."default",
            story text COLLATE pg_catalog."default",
            install_os text COLLATE pg_catalog."default",
            server_tstamp timestamp without time zone NOT NULL,  -- Add NOT NULL constraint
            client_tstamp timestamp without time zone,
            "time" bigint,
            message text COLLATE pg_catalog."default",
            device_name text COLLATE pg_catalog."default",
            device_model text COLLATE pg_catalog."default",
            total_ram integer,
			app_id text COLLATE pg_catalog."default"
        ) PARTITION BY RANGE (server_tstamp);', current_table);

        -- Create the parent partition
        EXECUTE format('SELECT partman.create_parent(
            p_parent_table => ''gsdk_stats.stats_count_%I'',
            p_control => ''server_tstamp'',      -- Column to partition by
            p_interval => ''5 days'',             -- Partitioning interval
            p_type => ''range'',                   -- Use time-based partitioning
            p_premake => 7
        );', current_table);
    END LOOP;
END $$;

-- Count all tables
DO $$
DECLARE
    table_names text[] := ARRAY[
        'flush_egress',
        'validate_token', 
        'parse',
        'recv_cb'
    ];
    current_table text;
    row_count bigint;
BEGIN
    FOREACH current_table IN ARRAY table_names
    LOOP
        -- Get the row count for the current table
        EXECUTE format('SELECT count(*) FROM gsdk_stats.stats_count_%I', current_table) INTO row_count;

        -- Print the result
        RAISE NOTICE 'Table % has % rows', current_table, row_count;
    END LOOP;
END $$;


/*
-- few sample queries
ALTER TABLE IF EXISTS gsdk_stats.stats_count_flush_egress
    OWNER to postgres;

REVOKE ALL ON TABLE gsdk_stats.stats_count_flush_egress FROM grafanareader;

GRANT SELECT ON TABLE gsdk_stats.stats_count_flush_egress TO grafanareader;

GRANT ALL ON TABLE gsdk_stats.stats_count_flush_egress TO postgres;

CREATE EXTENSION pg_partman;

SELECT * FROM pg_extension;
SELECT schema_name 
FROM information_schema.schemata;

CREATE SCHEMA partman;
SELECT * 
FROM information_schema.role_table_grants
WHERE table_schema = 'partman';

SELECT current_user;

GRANT USAGE ON SCHEMA partman TO postgres;

CREATE EXTENSION pg_partman WITH SCHEMA partman;

SELECT proname, proargtypes, prorettype
FROM pg_proc
WHERE proname = 'create_parent';

SELECT proname, proargtypes 
FROM pg_proc 
WHERE proname = 'create_parent';

-- Adding new column to all tables
DO $$
DECLARE
    table_names text[] := ARRAY['flush_egress',
                                'validate_token', 
                                'parse',
                                'recv_cb'];
    current_table text;
BEGIN
    FOREACH current_table IN ARRAY table_names
    LOOP
        -- Add a new column to each parent partitioned table
        EXECUTE format('ALTER TABLE gsdk_stats.stats_count_%I
                        ADD COLUMN IF NOT EXISTS app_id text;', current_table);
    END LOOP;
END $$;
*/

/*
TRUNCATE TABLE gsdk_stats.stats_count_flush_egress;
TRUNCATE TABLE gsdk_stats.stats_count_recv_cb;
TRUNCATE TABLE gsdk_stats.stats_count_validate_token;
TRUNCATE TABLE gsdk_stats.stats_count_parse;

select * from gsdk_stats.stats_count_flush_egress;
select * from gsdk_stats.stats_count_recv_cb;
select * from gsdk_stats.stats_count_validate_token;
select * from gsdk_stats.stats_count_parse;

SELECT * FROM pg_partition_tree('gsdk_stats.stats_count_flush_egress');
*/
