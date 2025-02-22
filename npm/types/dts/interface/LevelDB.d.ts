/// <reference path="../_import/_fibjs.d.ts" />
/// <reference path="../interface/object.d.ts" />
/// <reference path="../interface/Buffer.d.ts" />
/**
 * @description evelDB 是 fibjs 内置的数据库操作对象，用于创建和管理键值对形式的字典对象。使用 LevelDB 对象，可轻松实现键值对数据的存储、查询、删除、枚举等操作。它基于 Google 开源的 LevelDB 实现，具有高效、可靠、可扩展等优点
 * 
 * LevelDB 对象的创建非常简单，只需通过 db.openLevelDB 方法即可创建一个指定名称的数据库对象。例如：
 * 
 * ```JavaScript
 * var db = require("db");
 * var test = db.openLevelDB("test.db");
 * ```
 * 
 * 其中，db 为 fibjs 的数据库操作对象，openLevelDB 方法用于打开 leveldb 数据库，test.db 为数据库名称，函数返回的 test 对象即为操作数据库的对象。
 * 
 * LevelDB 对象支持的主要操作包括：
 * 
 * - set(key, value)：设置一个键值数据，键值不存在则插入新数据。
 * - get(key)：查询指定键值的值。
 * - has(key)：判断指定键值是否存在。
 * - remove(key)：删除指定键值的全部值。
 * - forEach(func)：枚举数据库中所有的键值对。
 * - between(from, to, func)：枚举数据库中键值在 from 和 to 之间的键值对。
 * - toJSON(key)：返回对象的 JSON 格式表示，一般返回对象定义的可读属性集合。
 * - begin()：在当前数据库上开启一个事务。
 * - commit()：提交当前事务。
 * - close()：关闭当前数据库连接或事务。
 * 
 * 例如：
 * 
 * ```JavaScript
 * var db = require("db");
 * var test = db.openLevelDB("test.db");
 * 
 * test.set("test_key", "test_value");
 * 
 * var value = test.get("test_key");
 * console.log("test_key:", value.toString());
 * 
 * test.remove("test_key");
 * 
 * console.log("has test_key:", test.has("test_key"));
 * 
 * test.close();
 * ```
 * 
 * 以上是 LevelDB 对象的基本用法及示例，可以方便灵活地操作键值对数据。在实际应用中，它可以被用于存储、缓存、日志等场景，提高数据读写效率、简化程序逻辑、减少开发复杂度等。
 *  
 */
declare class Class_LevelDB extends Class_object {
    /**
     * @description 检查数据库内是否存在指定键值的数据
     *      @param key 指定要检查的键值
     *      @return 返回键值是否存在
     *      
     */
    has(key: Class_Buffer): boolean;

    has(key: Class_Buffer, callback: (err: Error | undefined | null, retVal: boolean)=>any): void;

    /**
     * @description 查询指定键值的值
     *      @param key 指定要查询的键值
     *      @return 返回键值所对应的值，若不存在，则返回 null
     *      
     */
    get(key: Class_Buffer): Class_Buffer;

    get(key: Class_Buffer, callback: (err: Error | undefined | null, retVal: Class_Buffer)=>any): void;

    /**
     * @description 查询一组指定键值的值
     *      @param keys 指定要查询的键值数组
     *      @return 返回包含键值得数组
     *      
     */
    mget(keys: any[]): any[];

    /**
     * @description 设定一个键值数据，键值不存在则插入新数据
     *      @param key 指定要设定的键值
     *      @param value 指定要设定的数据
     *      
     */
    set(key: Class_Buffer, value: Class_Buffer): void;

    set(key: Class_Buffer, value: Class_Buffer, callback: (err: Error | undefined | null)=>any): void;

    /**
     * @description 设定一组键值数据，键值不存在则插入新数据
     *      @param map 指定要设定的键值数据字典
     *      
     */
    mset(map: FIBJS.GeneralObject): void;

    /**
     * @description 删除一组指定键值的值
     *      @param keys 指定要删除的键值数组
     *      
     */
    mremove(keys: any[]): void;

    /**
     * @description 删除指定键值的全部值
     *      @param key 指定要删除的键值
     *      
     */
    remove(key: Class_Buffer): void;

    remove(key: Class_Buffer, callback: (err: Error | undefined | null)=>any): void;

    /**
     * @description 枚举数据库中所有的键值对
     * 
     *      回调函数有两个参数，(value, key)
     * 
     *      ```JavaScript
     *      var db = require("db");
     *      var test = new db.openLevelDB("test.db");
     * 
     *      test.forEach(function(value, key){
     *         ...
     *      });
     *      ```
     *      @param func 枚举回调函数
     *      
     */
    forEach(func: (...args: any[])=>any): void;

    /**
     * @description 枚举数据库中键值在 from 和 to 之间的键值对
     * 
     *      回调函数有两个参数，(value, key)
     * 
     *      ```JavaScript
     *      var db = require("db");
     *      var test = new db.openLevelDB("test.db");
     * 
     *      test.between("aaa", "bbb", function(value, key){
     *         ...
     *      });
     *      ```
     *      @param from 枚举的最小键值，枚举时包含此键值
     *      @param to 枚举的最大键值，枚举时不包含此键值
     *      @param func 枚举回调函数
     *      
     */
    between(from: Class_Buffer, to: Class_Buffer, func: (...args: any[])=>any): void;

    /**
     * @description 在当前数据库上开启一个事务
     *      @return 返回一个开启的事务对象 
     */
    begin(): Class_LevelDB;

    /**
     * @description 提交当前事务 
     */
    commit(): void;

    /**
     * @description 关闭当前数据库连接或事务 
     */
    close(): void;

    close(callback: (err: Error | undefined | null)=>any): void;

}

