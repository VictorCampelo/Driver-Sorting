# SORT DRIVER
Receive n numbers and return a sorted list with this values
## Installation
Just execute shell script "commands.sh $1".

$1 is file name that is contains the code of driver

type: ```./commands.sh sortlist```
## Usage
To insert values to kernel space, type: ```echo -n "value" > /dev/sortlist``` 

Ex.:
```bash
echo -n 3 >/dev/sortlist
echo -n 1 >/dev/sortlist
echo -n 5 >/dev/sortlist
echo -n 7 >/dev/sortlist
echo -n 100 >/dev/sortlist
echo -n 44 >/dev/sortlist
echo -n 4 >/dev/sortlist
```

After insert your values to test, type: ```cat /dev/sortlist```

The result of the above entry is:

```bash
cat /dev/sortlist

1 3 4 5 7 44 100
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

1. Fork it!
2. Create your feature branch: `git checkout -b my-new-feature`
3. Commit your changes: `git commit -am 'Add some feature'`
4. Push to the branch: `git push origin my-new-feature`
5. Submit a pull request :D

Please make sure to update tests as appropriate.

## License
[MIT](https://choosealicense.com/licenses/mit/)
