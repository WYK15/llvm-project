//
//  Copyright © 2017年 shumei. All rights reserved.
//  Pingshun Wei<weipingshun@ishumei.com>
//

#import "SmStrUtils.h"

@implementation SmStrUtils

+(BOOL) empty:(NSString*) str {
    if (NULL == str) {
        return YES;
    }
    if (0 == [str length]) {
        return YES;
    }
    return NO;
}

+(BOOL) notEmpty:(NSString*) str {
    return ![self empty:str];
}

+(NSString*) trim:(NSString*) str {
    if (NULL == str) {
        return NULL;
    }
    return [str stringByTrimmingCharactersInSet:
                         [NSCharacterSet whitespaceCharacterSet]];
}

+(BOOL) blank:(NSString*) str {
    NSString *trimmed = [self trim: str];
    return [self empty:trimmed];
}

+(BOOL) notBlank:(NSString *)str {
    return ![self blank:str];
}

+(NSString*) safe:(NSString*) str {
    return [SmStrUtils notEmpty:str] ? str : @"";
}

+(BOOL) equal:(NSString*) left right:(NSString*) right {
    if (nil == left && nil == right) {
        return YES;
    }
    if (nil == left && nil != right) {
        return NO;
    }
    if (nil != left && nil == right) {
        return NO;
    }
    return [left isEqualToString:right];
}

+(BOOL) notEqual:(NSString*) left right:(NSString*) right {
    return ![self equal:left right:right];
}

+ (NSString *)randomString:(NSInteger)length {
    NSString *ramdom;
    NSMutableArray *array = [NSMutableArray array];
    for (int i = 0; i < length; i++) {
        int a = 97 + (arc4random_uniform(26));
        [array addObject:[NSString stringWithFormat:@"%c", (char) a]];
    }
    ramdom = [array componentsJoinedByString:@""];
    return ramdom;
}


+(NSString*) parseStr:(NSString*) str {
    if (str == nil) {
        return nil;
    }
    NSUInteger len = [str length];
    if (len % 2 != 0) {
        return str;
    }
    NSMutableString *result = [[NSMutableString alloc] init];
    for (size_t i = 0; i < len; i = i + 2) {
        NSString *sub = [str substringWithRange:NSMakeRange(i, 2)];
        int l = (int) strtol([sub cStringUsingEncoding:NSUTF8StringEncoding], NULL, 16);
        l = ~l;
        [result appendFormat:@"%c", l];
    }
    return result;
}

+ (NSString *) enc:(NSString *)str {
    if (str == nil) {
        return nil;
    }
    const char *cChar = [str cStringUsingEncoding:NSUTF8StringEncoding];
    size_t len = strlen(cChar);
    NSMutableString * result = [[NSMutableString alloc] init];
    for (size_t i = 0; i < len; i++) {
        int word = *(cChar + i);
        word = ~word;
        while (word < 0) {
            word += 256;
        }
        [result appendFormat:@"%02x", word];
    }
    return result;
}

@end
