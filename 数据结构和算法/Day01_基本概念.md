# 数据结构在学什么？

- 如何用程序代码把现实世界的问题**信息化**
- 如何用计算机高效地处理这些信息从而创造价值

# 绪论

## 数据结构的基本概念

1. **数据**：数据是<font color='red'>**信息的载体**</font>，是描述客观事物属性的数、字符及所有能输入到计算机中并<font color='red'>**被计算机程序识别和处理**</font>的符号的集合。
2. **数据元素**：描述一个个体。数据元素是数据的基本单位，通常作为一个整体进行考虑。
3. **数据项**：一个数据元素可以由多个数据项组成，数据项是构成数据元素的不可分割的最小单元
4. **数据对象**：具有<font color='red'>**相同性质**</font>的数据元素的集合，是数据的一个子集。
5. **数据结构**：是相互之间存在一种或多种<font color='red'>**特定关系**</font>的数据元素的集合。
6. **数据类型、抽象数据类型**(ADT)：是一个值的集合和定义在此集合上的一组操作的总称
   1. 原子类型：其值不可再分。int、bool
   2. 结构类型：其值可分解为若干成分的数据类型。struct{int x;int y;}

## 数据结构三要素

1. **逻辑结构**：集合、**线性（1-1）、树形（1-n）、图状（n-m）**
2. **数据的运算**：针对某种逻辑结构，结合实际需求，定义<font color='red'>**基本运算**</font>
3. **物理结构（存储结构）**：计算机如何实现这种数据结构的？
   1. 顺序存储：各个元素在物理上必须是连续的。
   2. 链式存储：借助指示元素存储地址的指针来表示元素之间的逻辑关系
   3. 索引存储：存储元素的同时，还建立附加的索引表。索引项（关键字，地址）
   4. 散列存储：根据元素的关键字直接计算出该元素的存储地址，又称哈希存储

## 什么是算法

有限时间内解决特定问题的一组指令或操作步骤。

## 算法的时间复杂度

**时间开销和问题规模n的关系：**一般来说，**算法的运行时间T和问题规模n有关**，他们之间存在某种函数关系，所以用n来表示时间开销。事前预估<font color='red'>**算法时间开销T(n)与问题规模n**</font>的关系。

只需要关注表达式最高阶。<u>**算法的性能问题只有在n很大时才会暴露出来**</u>。

- 加法规则：T(n)=T₁(n)+T₂(n)=O(f(n))+O(g(n))=O(max(f(n),g(n)))
- 乘法规则：T(n)=T₁(n)×T₂(n)=O(f(n))×O(g(n))=O(f(n)×g(n))

**常对幂指阶**：**<font color='red'>O(1)<O(log₂n)<O(n)<O(nlog₂n)<O(n²)<O(n³)<O(2ⁿ)<O(n!)<O(nⁿ)</font>**

1. <font color='green'>**顺序执行的代码可以忽略**</font>
2. 只需要关注<font color='red'>**循环**</font>中的<font color='green'><u>**一个基本操作分析它的执行次数和n的关系**</u></font>即可
3. 多层只需要关注<font color='red'>**最深层循环了几次**</font>。

- ```c
  void test(int n){
      int i = 1;//1次
      while(i <= n){//n+1次
          i++;//n次
          printf("...");//n次
      }
      printf("...");
  }//T(n)=O(1+n+1+2n+1)=O(n)
  ```

- ```c
  void test(int n){
      int i = 1;//1次
      while(i <= n){//当2^x>n时停止循环
          i*=2;//每次翻倍
          printf("...");//设x次  x=log₂n+1
      }
      printf("...");
  }//T(n)=O(log₂n)
  ```

## 算法的空间复杂度

**内存空间开销和问题规模n的关系：**经过编译形成机器指令放入内存

- 和问题规模n无关：无论问题规模怎么变，算法运行所需的内存空间是固定常量，则算法空间复杂度为**S(n)=O(1)**,即算法<font color='red'>**原地工作**</font>

- 和问题规模n有关：

  - ```c
    void test(int n){
        int flag[n];//定义的变量所占的空间n有关
        int i;//定义的变量所占的空间和n无关
        //...
    }//S(n)=O(4+4n+4)=O(n)
    ```

  - ```c
    void test(int n){
        int flag[n][n];//定义的变量所占的空间n有关
        int orther[n];
        int i;//定义的变量所占的空间和n无关
        //...
    }//S(n)=O(4+4n²+4n+4)=O(n²)
    ```

- **函数递归带来的内存开销**：下面这个**常规**例1：<font color='red'>**空间复杂度=递归调用的深度**</font>，因为每一层<font color='green'>**所需要的空间大小固定**</font>

  - ```c
    void test(int n){
        int a,b,c;
        if(n>1){
            test(n-1);
        }
        //...
    }//S(n)=O(4+4×3n)=O(n)
    ```

  - ```c
    void test(int n){
        int flag[n];//每一层所需要的空间大小不等
        if(n>1){
            test(n-1);
        }
        //...
    }//S(n)=O(1+2+...+n)=O(n²)
    ```





# 线性表

- 逻辑：具有<font color='red'>**相同类型**</font>的n个数据元素的<font color='red'>**有限序列**</font>(有先后次序)，n为表长
- 运算：**创销、增删改查**
  - 对参数的修改结果需要“带回来”。需要**引用&**
- 物理：**顺序存储、链式存储**

## 顺序表

**逻辑上相邻**的元素在**物理位置上也相邻**。用sizeof(e)知道数据元素的大小，可据此计算位置

- 静态分配：

  - **静态的数组**来存放数据，**数组大小无法改变**
  - 顺序表当前长度

- 基本操作：

  - 初始化线性表：没有设置数据元素的默认值，**内存中会有遗留的“脏数据”**。只有length=0即可。
  - 访问每一个元素，用**i<length**循环

- 动态分配：

  - 指示动态分配的指针
  - 最大容量
  - 长度

- 基本操作：

  - C——malloc、free

  - ```c
    L.data(ElemType*)malloc(sizeof(ElemType)*InitSize)//malloc函数返回一个指针，需要强制转型为你定义的数据元素类型指针，参数知名要分配多大连续内存空间，这个结果就是第一个元素的地址。申请一片连续的存储空间
    ```

  - **动态分配可以改变数组的长度**

  - ![image-20240922171833712](https://jjz125-1314914016.cos.ap-nanjing.myqcloud.com/typora/image-20240922171833712.png)







## 数组

### 创建和初始化数组

```javascript
let arr = new Array()//初始化一个数组
let arr = new Array(3)//指定长度
let arr = new Array('1','2','3')//指定元素
//推荐[]创建数组
let arr = ['1','2','3']
```

### 数组长度和遍历数组

```javascript
alert(arr.length)
//for
for(let i = 0;i < arr.length;i++){
    alert(arr[i])
}
//foreach
arr.forEach(function(value){
    alert(value)
})
```

### 数组常见操作

#### 添加元素

```javascript
arr.push('4')
arr.push('5','6','7')//添加元素到数组的最后位置

arr.unshift('-1')
arr.unshift('-2','-3')//首位添加元素

//-1 -2 -3 1 2 3 4 5 6 7
arr.splice(3,0,8,8,8,0)//从索引3开始，0表示添加，添加元素8，8，8，0
```

#### 删除元素

```js
arr.pop()//删除最后的元素

arr.shift()//删除首位元素

//-1 -2 -3 0 8 8 8 1 2 3 4 5 6 7
arr.splice(4,3)//从索引4开始删除2个元素
//-1 -2 -3 0 1 2 3 4 5 6 7
```

#### 数组合并

```js
let arr1 = [1,2,3]
let arr2 = [100,200,300]
let arr = arr1.concat(arr2)//方式1

arr= arr1+arr2//方式2
arr:[1,2,3,100,200,300]
```

#### 迭代方法

```js
//1.every()
let numbers = [10, 23, 56, 78, -5];
let allPositive = numbers.every(function(num) {
    return num > 0;//每一个元素都大于0返回true
});
console.log(allPositive); // false
//2.some()
let allPositive = numbers.some(function(num) {
    return num > 0;//有一个元素大于0返回true
});

//3.forEach()
numbers.forEach(function(t){
    alert(t);//遍历
})

//4.filter()——对每个元素执行function(num)并根据函数返回值决定是否保留
let evenNumbers = numbers.filter(function(num){
    return num % 2 === 0;//返回一个新数组
})

//5.map()
let doubled = numbers.map(function(num) {
    return num * 2;//将每一个元素乘2返回新数组
});

//6.reduce()——将数组中所有元素通过累积器函数进行归并，最终形成一个单一的值
let sum = numbers.reduce(function(accumulator,currentValue){
    return accumulator+currentValue;
},0);//初始值为0
//例二
let words = ['Hello', 'world', 'from', 'reduce'];
let sentence = words.reduce(function(accumulator, currentValue) {
    return accumulator + ' ' + currentValue;
});

console.log(sentence); // "Hello world from reduce"
优势在于reduce方法有返回值, 而forEach没有.那么reduce方法本身就可以作为参数直接传递
```



# 栈

只能在表的一个固定端进行数据节点的插入和删除操作：<font color='red'>**后进先出**</font>

## 栈结构实现

### 基于数组

```js
//封装栈类
function Stack(){
    //栈中的属性
    let items = []//基于数组，保存栈对象中所有的元素。无论是压栈还是出栈都是基于数组添加删除操作
    //...
    
    //压栈
    this.push = function(e){
        items.push(e)
    }
    
    //出栈
    this.pop = function(){
        return items.pop()
    }
    
    //peek操作
    this.peek = function(){
        return items[items.length-1]
    }
    
    //判断栈中的元素是否为空
    this.isEmpty = function(){
        return items.length===0
    }
    
    //获取栈中元素的个数
    this.size = function(){
        return items.length
    }
}
```



### 基于链表

























































































































































